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

    void ZoomView::Init(int width, int height, int posX, int posY)
    {
        cv::namedWindow(Name, WINDOW_NORMAL);
        cv::imshow(Name, MatView);
        cv::resizeWindow(Name, width, height);
        cv::setMouseCallback(Name, (cv::MouseCallback) DefaultMouseProcessor, this);
    }
    
    void ZoomView::DefaultKeyboardProcessor()
	{
		int c = waitKey(33);
		if ((c & 255) == 27)
		{
			cout << "Exiting ...\n";
		}
		switch ((char)c)
		{
		case 'c':
			break;
		case 'm':
			break;
		}
	}


	void ZoomView::ProcessEvents()
	{
		DefaultKeyboardProcessor();
		// randu(b, Scalar::all(0), Scalar::all(255));
	}


}