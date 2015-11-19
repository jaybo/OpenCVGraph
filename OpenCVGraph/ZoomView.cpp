#include "stdafx.h"
#include "ZoomView.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
    ZoomView * g_LastActiveZoomView = NULL;

	void ZoomView::DefaultMouseProcessor(int event, int x, int y, int flags, void* param)
	{
		ZoomView* view = (ZoomView*)param;
        g_LastActiveZoomView = view;

		cout << view->m_ZoomViewName << endl;

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
				view->m_ZoomFactor+= 2; 
			}
			else
			{
				view->m_ZoomFactor-=2;
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
                int absZoom = abs(view->m_ZoomFactor);
                if (view->m_ZoomFactor < 0) {
                    view->m_dx = (x - view->m_sx) * absZoom;
                    view->m_dy = (y - view->m_sy) * absZoom;
                }
                else  if (view->m_ZoomFactor > 0) {
                    view->m_dx = (int) ((x - view->m_sx) / (float) absZoom);
                    view->m_dy = (int) ((y - view->m_sy) / (float) absZoom);
                }
                else {
                    view->m_dx = (x - view->m_sx);
                    view->m_dy = (y - view->m_sy);
                }
            }
           // view->m_SampledPixelU16 = getU16Pix(view->MatView, P`oint(view->m_cx, view->m_cy));

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
        m_ZoomViewName = name;
		cout << m_ZoomViewName << endl;
	}

	ZoomView::~ZoomView()
	{
	}

    void ZoomView::Init(int width = 512, int height = 512, 
        cv::MouseCallback mouseCallback = NULL)
    {
        m_winWidth = width;
        m_winHeight = height;
        cv::namedWindow(m_ZoomViewName, WINDOW_AUTOSIZE);
        cv::resizeWindow(m_ZoomViewName, width, height);
        if (mouseCallback) {
            m_mouseCallback = mouseCallback;
        }
        cv::setMouseCallback(m_ZoomViewName, (cv::MouseCallback) DefaultMouseProcessor, this);
    }
    
    bool ZoomView::KeyboardProcessor(int key)
	{
        bool fOK = true;
 
        return fOK;
	}


	void ZoomView::processView(Mat mat, Mat matOverlay, GraphData& graphData, int zoomWindowLockIndex)
	{
        if (zoomWindowLockIndex >= 0) {
            if (this == g_LastActiveZoomView) {
                // We are the active view, so save our coords
                graphData.ZoomWindowPositions[zoomWindowLockIndex].x = m_cx;
                graphData.ZoomWindowPositions[zoomWindowLockIndex].y = m_cy;
                graphData.ZoomWindowPositions[zoomWindowLockIndex].dx = m_dx;
                graphData.ZoomWindowPositions[zoomWindowLockIndex].dy = m_dy;
                graphData.ZoomWindowPositions[zoomWindowLockIndex].zoomFactor = m_ZoomFactor;
            }
            else {
                // We aren't the active view, so sync to somebody else
                m_cx = graphData.ZoomWindowPositions[zoomWindowLockIndex].x;
                m_cy = graphData.ZoomWindowPositions[zoomWindowLockIndex].y;
                m_dx = graphData.ZoomWindowPositions[zoomWindowLockIndex].dx;
                m_dy = graphData.ZoomWindowPositions[zoomWindowLockIndex].dy;
                m_ZoomFactor = graphData.ZoomWindowPositions[zoomWindowLockIndex].zoomFactor;
            }
        }

        MatView = mat;
        if (firstTime) {
            firstTime = false;
            m_cx = MatView.cols / 2;
            m_cy = MatView.rows / 2;
        }

        int srcHeight, srcWidth;
        if (m_ZoomFactor >= 1) {
            srcHeight = (int)((float)m_winHeight / m_ZoomFactor);
            srcWidth = (int)((float)m_winWidth / m_ZoomFactor);
        }
        else if (m_ZoomFactor == 0) {
            srcHeight = m_winHeight;
            srcWidth = m_winWidth;
        }
        else {
            srcHeight = m_winHeight * abs(m_ZoomFactor);
            srcWidth = m_winWidth * abs(m_ZoomFactor);
        }

        getRectSubPix(MatView, Size(srcWidth, srcHeight), 
            Point(m_cx - m_dx, m_cy - m_dy),
            MatZoomed);

        resize(MatZoomed, MatZoomed, Size(m_winWidth, m_winHeight), 0, 0 , INTER_NEAREST);

        // merge in the (usually) text overlay
        if (!matOverlay.empty()) {
            // make a black outline to the text
            matOverlay.copyTo(MatShadow);
            dilate(MatShadow, MatShadow, dilateElement);
            bitwise_not(MatShadow, MatShadow);
            bitwise_and(MatShadow, MatZoomed, MatZoomed);
            bitwise_or(matOverlay, MatZoomed, MatZoomed);
        }
        cv::imshow(m_ZoomViewName, MatZoomed);
	}


}