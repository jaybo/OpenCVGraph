
#pragma once
#pragma warning(disable : 4482)

#include "..\stdafx.h"

namespace fs = ::boost::filesystem;

#include "..\Property.h"
#include "..\GraphData.h"
#include "..\Config.h"
#include "..\Filter.h"
#include "..\ZoomView.h"

namespace openCVGraph
{
    // General image source:
    //   if   "camera_index" is set, use that camera
    //   elif "image_name" is set, use just that image
    //   elif "movie_name" is set, use that movie
    //   elif "image_dir" is set and contains images, use all images in dir
    //   else create a noise image

    class CamDefault : public Filter
    {
    public:
        CamDefault(std::string name, GraphData& data, bool showView = false, int width = 512, int height= 512);

        virtual bool init(GraphData& graphData) override;
        virtual bool process(GraphData& graphData) override;
        virtual bool processKeyboard(GraphData& data, int key) override;
        virtual bool fini(GraphData& graphData) override;

        virtual void saveConfig(FileStorage& fs, GraphData& data) override;
        virtual void loadConfig(FileNode& fs, GraphData& data) override;

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
        std::string camera_index;
        std::string image_name;
        std::string movie_name;
        std::string image_dir;

        // Allow special setup parameters per camera
        std::string camera_name;        // Ximea8, Ximea16, ...

        cv::VideoCapture cap;
        vector<fs::path> images;
        int imageIndex = 0;

    private:

    };
}