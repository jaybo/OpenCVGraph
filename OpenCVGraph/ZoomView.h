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
        void UpdateView(cv::Mat mat, cv::Mat matOverlay, GraphData graphData);
		

	private:
		std::string Name;
		int m_cx = 0, m_cy = 0;     // center of view (in view image coords) (0,0 is center of view)
        int m_dx = 0, m_dy = 0;     // drag delta (window coords)
        int m_sx, m_sy;             // mouse pos at start of drag (window coords)
        int m_mx, m_my;             // current mouse position
        bool firstTime = true;

        int m_winWidth, m_winHeight;
        int ZoomFactor = 0;     // 0 is 1:1 pixelwise
        
        bool m_MouseLButtonDown = false;

        cv::Mat MatView;        // original image to view
        cv::Mat MatZoomed;      // then modified by pan and zoom
        
        cv::MouseCallback m_mouseCallback = NULL;
	};

}