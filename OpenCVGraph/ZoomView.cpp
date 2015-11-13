#include "stdafx.h"
#include "ZoomView.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{

	void ZoomView::DefaultMouseProcessor(int event, int x, int y, int flags, void* param)
	{
		ZoomView* view = (ZoomView*)param;
		cout << view->Name << endl;

        if (view->m_mouseCallback) {
            // call a user callback if supplied
            (*view->m_mouseCallback)(event, x, y, flags, param);
        }
		int d;

		switch (event)
		{
		case cv::EVENT_RBUTTONDOWN:
			break;
		case cv::EVENT_RBUTTONUP:
			break;

		case cv::EVENT_MOUSEWHEEL:
            view->m_mx = x;
            view->m_my = y;

			if (flags & EVENT_FLAG_CTRLKEY)
			{
				//sx = 1;
			}

			if (flags & EVENT_FLAG_SHIFTKEY)
			{
				//sy = 1;
			}

			if (!(flags & EVENT_FLAG_CTRLKEY) && !(flags & EVENT_FLAG_SHIFTKEY))
			{
				//sx = 1;
				//sy = 1;
			}

			d = getMouseWheelDelta(flags);
			cout << d << endl;
			if (d>0)
			{
				view->ZoomFactor++; 
			}
			else
			{
				view->ZoomFactor--;
			}
			break;

		case cv::EVENT_LBUTTONDOWN:
            view->m_MouseLButtonDown = true;
            view->m_sx = x;
            view->m_sy = y;
            view->m_dx = 0;
            view->m_dy = 0;
			break;

		case cv::EVENT_MOUSEMOVE:
			cout << x << ", " << y << endl;
            if (view->m_MouseLButtonDown) {
                int absZoom = abs(view->ZoomFactor);
                if (view->ZoomFactor < 0) {
                    view->m_dx = (x - view->m_sx) * absZoom;
                    view->m_dy = (y - view->m_sy) * absZoom;
                }
                else  if (view->ZoomFactor > 0) {
                    view->m_dx = (int) ((x - view->m_sx) / (float) absZoom);
                    view->m_dy = (int) ((y - view->m_sy) / (float) absZoom);
                }
                else {
                    view->m_dx = (x - view->m_sx);
                    view->m_dy = (y - view->m_sy);
                }
            }
			break;
		case cv::EVENT_LBUTTONUP:
            view->m_MouseLButtonDown = false;
            view->m_cx -= view->m_dx;
            view->m_cy -= view->m_dy;
            view->m_dx = 0;
            view->m_dy = 0;

			break;
		}

	}

	ZoomView::ZoomView() {

	}

	ZoomView::ZoomView(const string &name)
	{
		Name = name;
		cout << Name << endl;
	}

	ZoomView::~ZoomView()
	{
	}

    void ZoomView::Init(int width = 512, int height = 512, 
        cv::MouseCallback mouseCallback = NULL)
    {
        m_winWidth = width;
        m_winHeight = height;
        cv::namedWindow(Name, WINDOW_AUTOSIZE);
        cv::resizeWindow(Name, width, height);
        if (mouseCallback) {
            m_mouseCallback = mouseCallback;
        }
        cv::setMouseCallback(Name, (cv::MouseCallback) DefaultMouseProcessor, this);
    }
    
    bool ZoomView::KeyboardProcessor(int key)
	{
        bool fOK = true;
 
        return fOK;
	}


	void ZoomView::UpdateView(Mat mat, Mat matOverlay, GraphData graphData)
	{
        MatView = mat;
        if (firstTime) {
            firstTime = false;
            m_cx = MatView.cols / 2;
            m_cy = MatView.rows / 2;
        }

        int srcHeight, srcWidth;
        if (ZoomFactor >= 1) {
            srcHeight = (int)((float)m_winHeight / ZoomFactor);
            srcWidth = (int)((float)m_winWidth / ZoomFactor);
        }
        else if (ZoomFactor == 0) {
            srcHeight = m_winHeight;
            srcWidth = m_winWidth;
        }
        else {
            srcHeight = m_winHeight * abs(ZoomFactor);
            srcWidth = m_winWidth * abs(ZoomFactor);
        }

        getRectSubPix(MatView, Size(srcWidth, srcHeight), 
            Point(m_cx - m_dx, m_cy - m_dy),
            MatZoomed);
        resize(MatZoomed, MatZoomed, Size(m_winWidth, m_winHeight));

        // merge in the (usually) text overlay
        if (!matOverlay.empty()) {
            // MatZoomed += matOverlay;
            bitwise_or(MatZoomed, matOverlay, MatZoomed);
        }
        cv::imshow(Name, MatZoomed);
	}


}