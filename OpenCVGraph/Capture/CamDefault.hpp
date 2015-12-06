
#include "../stdafx.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
    class CamDefault : public Filter {
    public:
        // General image source:
        //   if   "camera_index" is set, use that camera
        //   elif "image_name" is set, use just that image
        //   elif "movie_name" is set, use that movie
        //   elif "image_dir" is set and contains images, use all images in dir
        //   else create a noise image

        CamDefault(std::string name, GraphData& graphData,
            int sourceFormat = -1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
            source = Noise;
        }

        //// keyWait required to make the UI activate
        //bool processKeyboard(GraphData& data, int key) override
        //{
        //    bool fOK = true;
        //    if (m_showView) {
        //        return m_ZoomView.KeyboardProcessor(key);  // Hmm,  what to do here?
        //    }
        //    return fOK;
        //}

        //Allocate resources if needed
        bool init(GraphData& graphData) override
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
                else if (graphData.m_NeedCV_8UC3) {
                    graphData.m_imCapture = Mat::zeros(m_width, m_height, CV_8UC3);
                }
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
                fOK = cap.read(graphData.m_imCapture);
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
                if (graphData.m_NeedCV_16UC1) {
                    cv::randu(graphData.m_imCapture, Scalar::all(0), Scalar::all(65536));
                }

                break;
            }

            graphData.CopyCaptureToRequiredFormats();


            return ProcessResult::OK;
        }


        void processView(GraphData& graphData) override
        {
            if (m_showView) {
                m_imView = graphData.m_imCapture;
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

        // following are processed in order, looking for valid input
        int camera_index = 0;
        std::string image_name;
        std::string movie_name;
        std::string image_dir;

        cv::VideoCapture cap;
        vector<string> images;
        int imageIndex = 0;
    };
}
