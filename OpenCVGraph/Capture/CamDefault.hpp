#pragma once

#include "../stdafx.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
    class CamDefault : public Filter, public ITemcaCamera {
    public:
        // General image source:
        //   if   "camera_index" is >= 0, use that OpenCV camera index
        //   elif "image_name" is set, use just that image
        //   elif "movie_name" is set, use that movie
        //   elif "image_dir" is set and contains images, use all images in dir
        //   else create a noise image
       
        CamDefault(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512, int format = CV_16U, 
            int cameraIndex =  0, string imageName = "", string movieName = "", string imageDir = "")
            : Filter(name, graphData, streamIn, width, height)
        {
            m_Format = format;
            source = Noise;

            construct_camera_index = cameraIndex;
            construct_image_name = imageName;
            construct_movie_name = movieName;
            construct_image_dir = imageDir;
        }

        //Allocate resources if needed
        bool init(GraphData& graphData) override
        {
            Filter::init(graphData);

            // if parms were passed to constructor, use them, else use YAML

            if (construct_camera_index != FROM_YAML) {
                camera_index = construct_camera_index;
            }
            if (construct_image_name != "") {
                image_name = construct_image_name;
            }
            if (construct_movie_name != "") {
                movie_name = construct_movie_name;
            }
            if (construct_image_dir != "") {
                image_dir = construct_image_dir;
            }

            bool fOK = false;

            // CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA 
            if (camera_index >= 0) {

                fOK = cap.open(camera_index);
                if (fOK) {
                    // set camera specific properties
                    fOK = cap.read(graphData.m_CommonData->m_imCapture);

                    if (!graphData.m_CommonData->m_imCapture.data)   // Check for invalid input
                    {
                        fOK = false;
                        graphData.m_Logger->error() << "Could not read from capture device #" << camera_index;
                    }
                    else {
                        source = Camera;
                        fOK = true;
                    }
                }
                else {
                    graphData.m_Logger->error() << "Could not open capture device #" << camera_index;
                }
            }

            // SingleImage SingleImage SingleImage SingleImage SingleImage SingleImage 
            if (!fOK && fileExists(image_name)) {
                Mat image = imread(image_name, CV_LOAD_IMAGE_UNCHANGED);   // Read the file

                if (!image.data)                              // Check for invalid input
                {
                    graphData.m_Logger->error() << "Could not open or find the image";
                }
                else {
                    graphData.m_CommonData->m_imCapture = image;
                    source = SingleImage;
                    graphData.m_CommonData->m_SourceFileName = image_name;
                    fOK = true;
                }
            }

            // Movie Movie Movie Movie Movie Movie Movie Movie Movie Movie Movie 
            if (!fOK && fileExists(movie_name)) {
                if (cap.open(movie_name)) {
                    source = Movie;
                    graphData.m_CommonData->m_SourceFileName = movie_name;
                    fOK = true;
                }
                else {
                    graphData.m_Logger->error() << "Could not open movie " << movie_name;
                }
            }

            // Directory Directory Directory Directory Directory Directory Directory 
            if (!fOK) {
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
                    graphData.m_Logger->error() << "Could not open directory" << image_dir;
                }
            }

            // Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise 
            if (!fOK) {
                source = Noise;
                graphData.m_CommonData->m_imCapture = Mat::zeros(m_ViewWidth, m_ViewHeight, m_Format);
                fOK = true;
            }

            return fOK;
        }


        ProcessResult process(GraphData& graphData) override
        {
            m_firstTime = false;
            bool fOK = true;

            switch (source) {
            case Camera:
                fOK = cap.read(graphData.m_CommonData->m_imCapture);
                break;
            case SingleImage:
                // nothing to do, already loaded
                break;
            case Movie:
                fOK = cap.read(graphData.m_CommonData->m_imCapture);
                break;
            case Directory:
                if (images.size() > 0) {
                    string fname = images[imageIndex];
                    // cout << fname << endl;
                    graphData.m_CommonData->m_SourceFileName = fname;
                    graphData.m_CommonData->m_imCapture = imread(fname, CV_LOAD_IMAGE_UNCHANGED);
                    imageIndex++;
                    if (imageIndex >= images.size()) {
                        imageIndex = 0;
                    }
                }
                break;
            case Noise:
                switch (m_Format)
                {
                case CV_8UC3:
                    break;
                case CV_8UC1:
                    cv::randu(graphData.m_CommonData->m_imCapture, Scalar::all(0), Scalar::all(255));
                    break;
                case CV_16UC1:
                    cv::randu(graphData.m_CommonData->m_imCapture, Scalar::all(0), Scalar::all(65535));
                    break;
                }
                break;
            }

            return ProcessResult::OK;
        }

        void processView(GraphData& graphData) override
        {
            if (m_showView) {
                m_imView = graphData.m_CommonData->m_imCapture;
                Filter::processView(graphData);
            }
        }

        // deallocate resources
        bool fini(GraphData& graphData) override
        {
            if (cap.isOpened()) {
                cap.release();
            }
            return true;
        }

        void  saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            cvWriteComment((CvFileStorage *)*fs, "Set camera_index to -1 to skip use of camera", 0);
            fs << "camera_index" << camera_index;
            cvWriteComment((CvFileStorage *)*fs, ".tiff, .png, or .jpg", 0);
            fs << "image_name" << image_name.c_str();
            fs << "movie_name" << movie_name.c_str();
            fs << "image_dir" << image_dir.c_str();
        }

        // Get setting either from YAML or constructor
        void  loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["camera_index"] >> camera_index;
            fs["image_name"] >> image_name;
            fs["movie_name"] >> movie_name;
            fs["image_dir"] >> image_dir;
        }

    protected:
        enum ImageSource {
            Camera,
            SingleImage,
            Movie,
            Directory,
            Noise,
        };

        ImageSource source;
        int m_Format;           // CV_8UC1, etc when synthesizing images.

        // following are processed in order, looking for valid input
        int camera_index = 0;
        std::string image_name;
        std::string movie_name;
        std::string image_dir;

        // Value passed in constructor which may direct to take the selection from YAML
        int construct_camera_index = 0;
        std::string construct_image_name;
        std::string construct_movie_name;
        std::string construct_image_dir;

        cv::VideoCapture cap;
        vector<string> images;
        int imageIndex = 0;
    };
}
