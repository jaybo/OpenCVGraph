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
			break;

		case cv::EVENT_MOUSEMOVE:
			cout << x << ", " << y << endl;
            if (view->m_MouseLButtonDown) {
                view->m_cx = -x;
                view->m_cy = -y;
            }
			break;
		case cv::EVENT_LBUTTONUP:
            view->m_MouseLButtonDown = false;
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
        m_cx = m_winWidth / 2;
        m_cy = m_winHeight / 2;
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


	void ZoomView::UpdateView(Mat mat)
	{
        MatView = mat;

        int srcHeight, srcWidth;
        if (ZoomFactor > 1) {
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
        Rect rSrc(srcWidth / 2 - m_cx, srcHeight / 2 - m_cy, srcWidth, srcHeight);

        cv::getRectSubPix(MatView, Size(srcWidth, srcHeight), Point(m_cx, m_cy), MatZoomed);
        resize(MatZoomed, MatZoomed, Size(m_winWidth, m_winHeight));
        cv::imshow(Name, MatZoomed);
	}


}