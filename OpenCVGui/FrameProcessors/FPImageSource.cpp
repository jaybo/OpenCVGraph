
#include "..\stdafx.h"
#include "..\FrameProcessor.h"
#include "FPImageSource.h"
#include <iomanip>

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

    FPImageSource::FPImageSource(std::string name, GraphData& data, bool showView)
        : FrameProcessor(name, data, showView)
    {
        source = Noise;
        camera_index = -1;
    }

    //Allocate resources if needed
    void FPImageSource::init(GraphData& data)
    {
        bool fOK = false;
        
        // CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA 
        if (camera_index.length() > 0) {

            std::stringstream(camera_index) >> cameraIndex;
            fOK = cap.open(cameraIndex);
            cap.read(data.imCapture);

            if (!data.imCapture.data)   // Check for invalid input
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
            Mat image = imread(image_name, CV_LOAD_IMAGE_COLOR);   // Read the file

            if (!image.data)                              // Check for invalid input
            {
                std::cout << "Could not open or find the image" << std::endl;
            }
            else {
                data.imCapture = image;
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

    }


    bool FPImageSource::process(GraphData& data)
    {
        firstTime = false;
        // data.imCapture =
        return true;
    }

    // deallocate resources
    void FPImageSource::fini(GraphData& data)
    {

    }



    void  FPImageSource::saveConfig() 
    {
        FileStorage fs(persistFile, FileStorage::WRITE);
        fs << "tictoc" << tictoc.c_str();
        fs << "camera_index" << image_name.c_str();
        fs << "image_name" << image_name.c_str();
        fs << "movie_name" << image_name.c_str();
        fs << "image_dir" << image_dir.c_str();

        fs.release();
    }

    void  FPImageSource::loadConfig()
    {
        FileStorage fs2(persistFile, FileStorage::READ);

        fs2["tictoc"] >> tictoc;
        fs2["camera_index"] >> camera_index;
        fs2["image_name"] >> image_name;
        fs2["movie_name"] >> movie_name;
        fs2["image_dir"] >> image_dir;

        fs2.release();
    }
}
