
#include "..\stdafx.h"


using namespace std;
using namespace cv;
namespace fs = ::boost::filesystem;

#include "..\FrameProcessor.h"
#include "FPImageSource.h"
#include "xiapi.h"


namespace openCVGui
{
    // General image source:
    //   if   "camera_index" is set, use that camera
    //   elif "image_name" is set, use just that image
    //   elif "movie_name" is set, use that movie
    //   elif "image_dir" is set and contains images, use all images in dir
    //   else create a noise image

    FPImageSource::FPImageSource(std::string name, GraphData& graphData, bool showView)
        : FrameProcessor(name, graphData, showView)
    {
        source = Noise;
        camera_index = -1;
    }

    //Allocate resources if needed
    bool FPImageSource::init(GraphData& graphData)
    {
		// call the base to read/write configs
		FrameProcessor::init(graphData);

        bool fOK = false;
        
        // CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA 
        if (camera_index.length() > 0) {

            std::stringstream(camera_index) >> cameraIndex;
            fOK = cap.open(cameraIndex);
            if (fOK) {
                // set camera specific properties
                if (camera_name == "Ximea16") {

                    // only capture 3840x3840@16bpp
                    fOK = cap.set(CV_CAP_PROP_XI_IMAGE_DATA_FORMAT, XI_MONO16);
                    fOK = cap.set(CV_CAP_PROP_FRAME_WIDTH, 3840);
                    fOK = cap.set(CV_CAP_PROP_XI_OFFSET_X, 640);

                    // Autogain off
                    fOK = cap.set(CV_CAP_PROP_XI_AEAG, 0);  

                    // Enable aperature and focus
                    cap.set(CV_CAP_PROP_XI_LENS_MODE, 1);
                    focalDistance = cap.get(CV_CAP_PROP_XI_LENS_FOCUS_DISTANCE);
                    focalLength = cap.get(CV_CAP_PROP_XI_LENS_FOCAL_LENGTH);
                    aperatureValue = cap.get(CV_CAP_PROP_XI_LENS_APERTURE_VALUE);

                    fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE, 10);
                    focusMovementValue = cap.get(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE);

                }
                fOK = cap.read(graphData.imCapture);

                if (!graphData.imCapture.data)   // Check for invalid input
                {
                    fOK = false;
                    std::cout << "Could not open capture device #" << camera_index << std::endl;
                }
                else {
                    source = Camera;
                    fOK = true;
                }
            }
        }
        
        // SingleImage SingleImage SingleImage SingleImage SingleImage SingleImage 
        if (!fOK && fileExists(image_name)) {
            Mat image = imread(image_name, CV_LOAD_IMAGE_UNCHANGED);   // Read the file

            if (!image.data)                              // Check for invalid input
            {
                std::cout << "Could not open or find the image" << std::endl;
            }
            else {
                graphData.imCapture = image;
                source = SingleImage;
                fOK = true;
            }
        }
        
        // Movie Movie Movie Movie Movie Movie Movie Movie Movie Movie Movie 
        if (!fOK && fileExists(movie_name)) {
            if (cap.open(movie_name)) {
                source = Movie;
                fOK = true;
            }
        }

        // Directory Directory Directory Directory Directory Directory Directory 
        if (!fOK) {
            if (fs::exists(image_dir) && fs::is_directory(image_dir)) {

                fs::recursive_directory_iterator it(image_dir);
                fs::recursive_directory_iterator endit;
                vector<string> extensions = { ".png", ".tif", ".tiff", ".jpg", ".jpeg" };
                while (it != endit)
                {
                    string lower = it->path().extension().string();
                    // tolower and replace those pesky backslashes
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    std::replace(lower.begin(), lower.end(), '\\', '/');
                    cout << it->path().extension() << endl;

                    if (fs::is_regular_file(*it) &&
                        std::find(extensions.begin(), extensions.end(), lower) != extensions.end())
                    {
                        images.push_back(it->path());
                    }
                    ++it;
                }
                source = Directory;
                fOK = true;
            }
        }
		
		// Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise 
		if (!fOK) {
			source = Noise;
			graphData.imCapture = Mat::zeros(512, 512, CV_16U);
			fOK = true;
		}

		return fOK;
    }


    bool FPImageSource::process(GraphData& graphData)
    {
        firstTime = false;
		bool fOK = true;

		switch (source) {
		case Camera:
			fOK = cap.read (graphData.imCapture);
			break;
		case SingleImage:
			// nothing to do, already loaded
			break;
		case Movie:
			fOK = cap.read(graphData.imCapture);
			break;
		case Directory:
            if (images.size() > 0) {
                string fname = images[imageIndex].string();
                // cout << fname << endl;
                graphData.imCapture = imread(fname);
                imageIndex++;
                if (imageIndex >= images.size()) {
                    imageIndex = 0;
                }
            }
			break;
		case Noise:
			 //cv::randu(graphData.imCapture, Scalar::all(0), Scalar::all(65536));
            //cv::randu(graphData.imCapture, Scalar::all(0), Scalar::all(255));
            //graphData.imCapture = Mat::zeros(512, 512, CV_16U);
            cv::randu(graphData.imCapture, Scalar::all(0), Scalar::all(65536));
            break;
		}

		if (showView && fOK) {
			imView = graphData.imCapture;
			cv::imshow(CombinedName, imView);
		}
        return fOK;
    }

    // deallocate resources
    bool FPImageSource::fini(GraphData& graphData)
    {
		if (cap.isOpened()) {
			cap.release();
		}
		return true;
    }



    void  FPImageSource::saveConfig() 
    {
        FileStorage fs2(persistFile, FileStorage::WRITE);
        fs2 << "tictoc" << tictoc;
        fs2 << "camera_index" << camera_index;
        fs2 << "camera_name" << camera_name;
        fs2 << "image_name" << image_name;
        fs2 << "movie_name" << movie_name;
        fs2 << "image_dir" << image_dir;

        fs2.release();
    }

    void  FPImageSource::loadConfig()
    {
        FileStorage fs2(persistFile, FileStorage::READ);
		cout << persistFile << endl;

        fs2["tictoc"] >> tictoc;
        fs2["camera_index"] >> camera_index;
        fs2 ["camera_name"] >> camera_name;
        fs2["image_name"] >> image_name;
        fs2["movie_name"] >> movie_name;
        fs2["image_dir"] >> image_dir;

        fs2.release();
    }
}
