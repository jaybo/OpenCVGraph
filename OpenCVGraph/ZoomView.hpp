#pragma once

#include "stdafx.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
#define MAX_ZOOMVIEW_LOCKS 10

    class ZoomView;
    static ZoomView * g_LastActiveZoomView = NULL;

    struct ZoomWindowPosition
    {
        int x = 0;              // center of image
        int y = 0;
        int dx = 0;             // delta from center due to mouse dragging
        int dy = 0;
        int zoomFactor = 0;
    };
    // Lock zoom and scroll positions of different filters
    static ZoomWindowPosition ZoomWindowPositions[MAX_ZOOMVIEW_LOCKS];     // Lock ZoomWindows

    // Displays Mats with zooming and synchronized pan/scroll between different windows
    //
    class ZoomView
    {
    public:
        static void DefaultMouseProcessor(int event, int x, int y, int flags, void* param)
        {
            ZoomView* view = (ZoomView*)param;
            g_LastActiveZoomView = view;

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
            {
                view->m_mx = x;
                view->m_my = y;

                //if (flags & EVENT_FLAG_CTRLKEY)
                //{
                //}

                //if (flags & EVENT_FLAG_SHIFTKEY)
                //{
                //}

                //if (!(flags & EVENT_FLAG_CTRLKEY) && !(flags & EVENT_FLAG_SHIFTKEY))
                //{
                //}

                // zoom faster on big images
                int zoomInc = (view->MatView.size().width >= 1024) ? 2 : 1;
                d = getMouseWheelDelta(flags);
                cout << d << endl;
                if (d > 0)
                {
                    view->m_ZoomFactor += zoomInc;
                }
                else
                {
                    view->m_ZoomFactor -= zoomInc;
                }
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
                // cout << x << ", " << y << endl;
                if (view->m_MouseLButtonDown) {
                    int absZoom = abs(view->m_ZoomFactor);
                    if (view->m_ZoomFactor < 0) {
                        view->m_dx = (x - view->m_sx) * absZoom;
                        view->m_dy = (y - view->m_sy) * absZoom;
                    }
                    else  if (view->m_ZoomFactor > 0) {
                        view->m_dx = (int)((x - view->m_sx) / (float)absZoom);
                        view->m_dy = (int)((y - view->m_sy) / (float)absZoom);
                    }
                    else {
                        view->m_dx = (x - view->m_sx);
                        view->m_dy = (y - view->m_sy);
                    }
                }
                // cout << view->m_dx << " " << view->m_cx << endl;
                view->m_wx = x;
                view->m_wy = y;
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

        ZoomView() {
        }

        ZoomView(const string &name)
        {
            m_ZoomViewName = name;
        }

        ~ZoomView()
        {
        }

        void Init(int width = 512, int height = 512,
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

        bool KeyboardProcessor(int key)
        {
            bool fOK = true;
            return fOK;
        }


        void processView(Mat mat, Mat matOverlay, GraphData& graphData, int zoomWindowLockIndex)
        {
            if (zoomWindowLockIndex >= 0) {
                if (this == g_LastActiveZoomView) {
                    // We are the active view, so save our coords
                    ZoomWindowPositions[zoomWindowLockIndex].x = m_cx;
                    ZoomWindowPositions[zoomWindowLockIndex].y = m_cy;
                    ZoomWindowPositions[zoomWindowLockIndex].dx = m_dx;
                    ZoomWindowPositions[zoomWindowLockIndex].dy = m_dy;
                    ZoomWindowPositions[zoomWindowLockIndex].zoomFactor = m_ZoomFactor;
                }
                else {
                    // We aren't the active view, so sync to somebody else
                    m_cx = ZoomWindowPositions[zoomWindowLockIndex].x;
                    m_cy = ZoomWindowPositions[zoomWindowLockIndex].y;
                    m_dx = ZoomWindowPositions[zoomWindowLockIndex].dx;
                    m_dy = ZoomWindowPositions[zoomWindowLockIndex].dy;
                    m_ZoomFactor = ZoomWindowPositions[zoomWindowLockIndex].zoomFactor;
                }
            }

            MatView = mat;
            if (firstTime) {
                firstTime = false;
                m_cx = MatView.cols / 2;
                m_cy = MatView.rows / 2;
                m_dx = 0;
                m_dy = 0;
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

            srcWidth = min(srcWidth, MatView.size().width);
            srcHeight = min(srcHeight, MatView.size().height);

            //Point tl(m_cx - m_dx, m_dy - m_cy); // center of view
            Point tl(m_cx - m_dx, m_cy - m_dy); // center of view
            tl.x -= srcWidth / 2;                 // top left
            tl.x = min(tl.x, MatView.size().width - srcWidth);
            tl.x = max(0, tl.x);
            //cout << tl.y;
            tl.y -= srcHeight / 2;
            tl.y = min(tl.y, MatView.size().height - srcHeight);
            tl.y = max(0, tl.y);

            MatZoomed = MatView(Rect(tl, Size(srcWidth, srcHeight)));

            cv::resize(MatZoomed, MatZoomed, Size(m_winWidth, m_winHeight), 0, 0, INTER_NEAREST);

            int nChannels = MatZoomed.channels();
            int nDepth = MatZoomed.depth();
            // 1if 16bpp, convert to 8bpp
            if (nDepth == 2 && nChannels == 1) {
                MatZoomed.convertTo(MatZoomed, CV_8UC1, 1.0 / 256);
            }
            if (nChannels != 3) {
                cv::cvtColor(MatZoomed, MatZoomed, CV_GRAY2RGB);
            }

            // merge in the (usually) text overlay
            if (!matOverlay.empty()) {
                // make a black outline to the text
                matOverlay.copyTo(MatShadow);
                dilate(MatShadow, MatShadow, dilateElement, Point(-1, -1), 1);
                cv::bitwise_not(MatShadow, MatShadow);
                cv::bitwise_and(MatShadow, MatZoomed, MatZoomed);
                cv::bitwise_or(matOverlay, MatZoomed, MatZoomed);
            }
            cv::imshow(m_ZoomViewName, MatZoomed);
        }

    private:
        std::string m_ZoomViewName;
        int m_cx = 0, m_cy = 0;     // center of view (in view image coords) (0,0 is center of view)
        int m_dx = 0, m_dy = 0;     // drag delta (window coords)
        int m_wx = 0, m_wy = 0;     // window coords
        int m_sx, m_sy;             // mouse pos at start of drag (window coords)
        int m_mx, m_my;             // current mouse position
        bool firstTime = true;

        int m_winWidth, m_winHeight;
        int m_ZoomFactor = 0;     // 0 is 1:1 pixelwise
        bool m_MouseLButtonDown = false;
        int m_SampledPixelU16;

        cv::Mat MatView;        // original image to view
        cv::Mat MatZoomed;      // then modified by pan and zoom
        cv::Mat MatShadow;
        int dilation_size = 1;
        Mat dilateElement = cv::getStructuringElement(MORPH_RECT,
            Size(2 * dilation_size + 1, 2 * dilation_size + 1),
            Point(dilation_size, dilation_size));
        cv::MouseCallback m_mouseCallback = NULL;
    };
}

