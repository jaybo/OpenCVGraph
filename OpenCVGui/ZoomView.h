#pragma once


namespace openCVGui
{
	class  ZoomView
	{

	public:
		ZoomView();
		ZoomView(const std::string &name, cv::Mat &mat);

		~ZoomView();

        void Init(int width, int height, int posX, int posY);
		void ProcessEvents();
		virtual bool KeyboardProcessor();
		static void DefaultMouseProcessor(int event, int x, int y, int flags, void* param);

		int ZoomFactor = 1;

	private:
		std::string Name;
		int sx = 0, sy = 0;
		cv::Mat MatView;

	};

}