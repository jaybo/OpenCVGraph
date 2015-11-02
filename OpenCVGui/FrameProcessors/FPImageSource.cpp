
#include "..\stdafx.h"
#include "..\FrameProcessor.h"
#include "FPImageSource.h"
#include <iomanip>

using namespace std;
using namespace openCVGui;
using namespace cv;

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
            cap.read(graphData.imCapture);

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
        if (!fOK && dirExists(image_dir)) {
            if (cap.open(movie_name)) {
                source = Directory;
                fOK = true;
            }
        }
		
		// Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise 
		if (!fOK) {
			source = Noise;
			graphData.imCapture = Mat::eye(1024, 1024, CV_16U);
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
			break;
		case Noise:
			cv::randu(graphData.imCapture, Scalar::all(0), Scalar::all(64000));
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
        FileStorage fs(persistFile, FileStorage::WRITE);
        fs << "tictoc" << tictoc;
        fs << "camera_index" << camera_index;
        fs << "image_name" << image_name;
        fs << "movie_name" << movie_name;
        fs << "image_dir" << image_dir;

        fs.release();
    }

    void  FPImageSource::loadConfig()
    {
        FileStorage fs2(persistFile, FileStorage::READ);
		cout << persistFile << endl;

        fs2["tictoc"] >> tictoc;
        fs2["camera_index"] >> camera_index;
        fs2["image_name"] >> image_name;
        fs2["movie_name"] >> movie_name;
        fs2["image_dir"] >> image_dir;

        fs2.release();
    }
}
