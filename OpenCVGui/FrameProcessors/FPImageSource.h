
#pragma once
#pragma warning(disable : 4482)

#include "..\stdafx.h"

namespace fs = ::boost::filesystem;

#include "..\Property.h"
#include "..\GraphData.h"
#include "..\Config.h"
#include "..\FrameProcessor.h"
#include "..\ZoomView.h"

namespace openCVGui
{
    // General image source:
    //   if   "camera_index" is set, use that camera
    //   elif "image_name" is set, use just that image
    //   elif "movie_name" is set, use that movie
    //   elif "image_dir" is set and contains images, use all images in dir
    //   else create a noise image

    class FPImageSource : public FrameProcessor
    {
    public:
        FPImageSource(std::string name, GraphData& data, bool showView = false);

        virtual bool init(GraphData& graphData) override;
        virtual bool process(GraphData& graphData) override;
        virtual bool fini(GraphData& graphData) override;

        virtual void saveConfig() override;
        virtual void loadConfig() override;

    private:
        enum ImageSource {
            Camera,
            SingleImage,
            Movie,
            Directory,
            Noise,
        };

        ImageSource source;

        // following are processed in order, looking for valid input
        std::string camera_index;
        std::string image_name;
        std::string movie_name;
        std::string image_dir;

        // Allow special setup parameters per camera
        std::string camera_name;        // Ximea8, Ximea16, ...

        int cameraIndex;
        cv::VideoCapture cap;
        vector<fs::path> images;
        int imageIndex = 0;

        double focalDistance;
        double focalLength;
        double aperatureValue;
        double focusMovementValue;
    };
}