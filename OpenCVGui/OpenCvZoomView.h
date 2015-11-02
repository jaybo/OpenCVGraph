#pragma once


namespace openCVGui
{
	class  OpenCvZoomView
	{

	public:
		OpenCvZoomView(const std::string &name, cv::Mat &mat, int width, int height, int posX, int posY);

		~OpenCvZoomView();

		void OpenCvZoomView::ProcessEvents();
		void OpenCvZoomView::DefaultKeyboardProcessor();
		static void DefaultMouseProcessor(int event, int x, int y, int flags, void* param);
		int ZoomFactor = 1;

	private:
		std::string Name;
		int sx = 0, sy = 0;
		cv::Mat MatView;

	};

}