#pragma once


namespace openCVGraph
{

    typedef bool (*KeyboardCallback)(int key);

	class  ZoomView
	{

	public:
		ZoomView();
		ZoomView(const std::string &name);

		~ZoomView();

        void Init(int width, int height, 
            cv::MouseCallback mouseCallback);
		virtual bool KeyboardProcessor(int key);
		static void DefaultMouseProcessor(int event, int x, int y, int flags, void* param);
        void UpdateView(cv::Mat mat);
		

	private:
		std::string Name;
		int m_cx, m_cy;
        int m_winWidth, m_winHeight;
        int ZoomFactor = 0;     // 0 is 1:1 pixelwise
        
        bool m_MouseLButtonDown;

        cv::Mat MatView;        // original image to view
        cv::Mat MatZoomed;      // then modified by pan and zoom
        
        cv::MouseCallback m_mouseCallback = NULL;
	};

}