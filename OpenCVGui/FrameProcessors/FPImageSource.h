
#pragma once
#pragma warning(disable : 4482)

#include "..\stdafx.h"

#include "..\Property.h"
#include "..\GraphData.h"
#include "..\Config.h"
#include "..\FrameProcessor.h"
#include "..\OpenCvZoomView.h"

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

		virtual void init(GraphData& data) override;
		virtual bool process(GraphData& data) override;
		virtual void fini(GraphData& data) override;

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

        int cameraIndex;
        cv::VideoCapture cap;

	};
}
