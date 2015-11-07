#include "stdafx.h"
#include "ZoomView.h"

using namespace std;
using namespace cv;

namespace openCVGui
{

	void ZoomView::DefaultMouseProcessor(int event, int x, int y, int flags, void* param)
	{
		int j = 42;

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
			break;

		case cv::EVENT_MOUSEMOVE:
			cout << x << ", " << y << endl;
			break;
		case cv::EVENT_LBUTTONUP:
			break;
		}
	}

	ZoomView::ZoomView() {

	}

	ZoomView::ZoomView(const string &name, cv::Mat &mat)
	{
		Name = name;
		cout << Name << endl;
		MatView = mat;
        
	}



	ZoomView::~ZoomView()
	{
	}

    void ZoomView::Init(int width = 512, int height = 512, int posX = 0, int posY = 0, 
        cv::MouseCallback mouseCallback = NULL)
    {
        cv::namedWindow(Name, WINDOW_NORMAL);
        cv::imshow(Name, MatView);
        cv::resizeWindow(Name, width, height);
        cv::moveWindow(Name, posX, posY);
        if (mouseCallback) {
            m_mouseCallback = mouseCallback;
        }
        cv::setMouseCallback(Name, (cv::MouseCallback) DefaultMouseProcessor, this);
    }
    
    bool ZoomView::KeyboardProcessor()
	{
        bool fOK = true;
        int c = waitKey(1);
        if (c != -1) {
            if ((c & 255) == 27)
            {
                cout << Name << " exit via ESC\n";
                fOK = false;
            }
        }
        return fOK;
	}


	void ZoomView::ProcessEvents()
	{
		KeyboardProcessor();
	}


}