
#include "../stdafx.h"
#include "CamDefault.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
    // General image source:
    //   if   "camera_index" is set, use that camera
    //   elif "image_name" is set, use just that image
    //   elif "movie_name" is set, use that movie
    //   elif "image_dir" is set and contains images, use all images in dir
    //   else create a noise image

    CamDefault::CamDefault(std::string name, GraphData& graphData, int width, int height)
        : openCVGraph::Filter(name, graphData, width, height)
    {
        source = Noise;
    }

    // keyWait required to make the UI activate
    bool CamDefault::processKeyboard(GraphData& data, int key)
    {
        bool fOK = true;
        if (m_showView) {
            return m_ZoomView.KeyboardProcessor(key);  // Hmm,  what to do here?
        }
        return fOK;
    }

    //Allocate resources if needed
    bool CamDefault::init(GraphData& graphData)
    {
		// call the base to read/write configs
		Filter::init(graphData);

        bool fOK = false;
        
        // CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA 
        if (camera_index >= 0) {

            fOK = cap.open(camera_index);
            if (fOK) {
                // set camera specific properties
                fOK = cap.read(graphData.m_imCapture);

                if (!graphData.m_imCapture.data)   // Check for invalid input
                {
                    fOK = false;
                    std::cout << "Could not read from capture device #" << camera_index << std::endl;
                }
                else {
                    source = Camera;
                    fOK = true;
                }
            }
            else {
                std::cout << "Could not open capture device #" << camera_index << std::endl;
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
                graphData.m_imCapture = image;
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
#if true
            DIR *dir;
            vector<string> extensions = { ".png", ".tif", ".tiff", ".jpg", ".jpeg" };
            struct dirent *ent;
            if ((dir = opendir(image_dir.c_str())) != NULL) {
                /* find files with matching extensions.  C++ is sooooo ugly for this kind of stuff */
                string imageDirWithEndingSlash = image_dir;
                if (!(image_dir.back() == '/')) {
                    imageDirWithEndingSlash += "/";
                }
                while ((ent = readdir(dir)) != NULL) {
                    if (ent->d_type == DT_REG) {
                        string originalCase = ent->d_name;
                        string lowerCase = ent->d_name;
                        std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), ::tolower);

                        for (std::vector<string>::iterator it = extensions.begin(); it != extensions.end(); ++it) {
                            string extension = *it;
                            if (lowerCase.find(extension, (lowerCase.length() - extension.length())) != std::string::npos)
                            {
                                string fullPath = imageDirWithEndingSlash + ent->d_name;
                                images.push_back(fullPath);
                                //printf("%s\n", fullPath.c_str());
                                fOK = true;
                            }
                        }
                    }
                }
                closedir(dir);
                source = Directory;
                fOK = true;
            }
            else {
                /* could not open directory */
            }
#else
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
#endif
        }
		
		// Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise 
		if (!fOK) {
			source = Noise;
            if (graphData.m_NeedCV_16UC1) {
                graphData.m_imCapture = Mat::zeros(m_width, m_height, CV_16UC1);
            }
            if (graphData.m_NeedCV_8UC3) {
                graphData.m_imCapture = Mat::zeros(m_width, m_height, CV_8UC3);
            }
            fOK = true;
		}

		return fOK;
    }


    ProcessResult CamDefault::process(GraphData& graphData)
    {
        m_firstTime = false;
		bool fOK = true;

		switch (source) {
		case Camera:
			fOK = cap.read (graphData.m_imCapture);
			break;
		case SingleImage:
			// nothing to do, already loaded
			break;
		case Movie:
			fOK = cap.read(graphData.m_imCapture);
			break;
		case Directory:
            if (images.size() > 0) {
                string fname = images[imageIndex];
                // cout << fname << endl;
                graphData.m_imCapture = imread(fname);
                imageIndex++;
                if (imageIndex >= images.size()) {
                    imageIndex = 0;
                }
            }
			break;
		case Noise:
			 //cv::randu(graphData.m_imCapture, Scalar::all(0), Scalar::all(65536));
            //cv::randu(graphData.m_imCapture, Scalar::all(0), Scalar::all(255));
            //graphData.m_imCapture = Mat::zeros(512, 512, CV_16U);
            cv::randu(graphData.m_imCapture, Scalar::all(0), Scalar::all(65536));
            break;
		}
        // Copy imCapture to imResult
        graphData.m_imCapture.copyTo(graphData.m_imResult8U);

        if (graphData.m_UseCuda) {
            cvtColor(graphData.m_imCapture, graphData.m_imCapture8U, CV_RGB2GRAY);
            graphData.m_imCaptureGpu8U.upload(graphData.m_imCapture8U);
            cv::cuda::lshift(graphData.m_imCapture8U, 8, graphData.m_imCapture8U);
        }
        else {
            if (graphData.m_NeedCV_16UC1) {
                graphData.m_imCapture16U = graphData.m_imCapture;
                graphData.m_imResult16U = graphData.m_imCapture;
                graphData.m_imCapture.convertTo(graphData.m_imResult8U, CV_8U);
            }
            if (graphData.m_NeedCV_8UC3) {
                cvtColor(graphData.m_imCapture, graphData.m_imCapture8U, CV_RGB2GRAY);
            }

        }

        return ProcessResult::OK;
    }


    void CamDefault::processView(GraphData& graphData)
    {
        if (m_showView) {
            m_imView = graphData.m_imCapture;
        //    Filter::processView(graphData);
        }
    }

    // deallocate resources
    bool CamDefault::fini(GraphData& graphData)
    {
		if (cap.isOpened()) {
			cap.release();
		}
		return true;
    }



    void  CamDefault::saveConfig(FileStorage& fs, GraphData& data) 
    {
        Filter::saveConfig(fs, data);
        cvWriteComment((CvFileStorage *) *fs, "Set camera_index to -1 to skip use of camera", 0);
        fs << "camera_index" << camera_index;
        cvWriteComment((CvFileStorage *)*fs, ".tiff, .png, or .jpg", 0);
        fs << "image_name" << image_name.c_str();
        fs << "movie_name" << movie_name.c_str();
        fs << "image_dir" << image_dir.c_str();
    }

    void  CamDefault::loadConfig(FileNode& fs, GraphData& data) 
    {
        Filter::loadConfig(fs, data);
        fs["camera_index"] >> camera_index;
        fs["image_name"] >> image_name;
        fs["movie_name"] >> movie_name;
        fs["image_dir"] >> image_dir;
    }
}
