#include "stdafx.h"
#include "OpenCVZoomView.h"

using namespace std;
using namespace cv;

namespace openCVGui
{

	void OpenCvZoomView::DefaultMouseProcessor(int event, int x, int y, int flags, void* param)
	{
		int j = 42;

		OpenCvZoomView* view = (OpenCvZoomView*)param;
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

	OpenCvZoomView::OpenCvZoomView() {

	}

	OpenCvZoomView::OpenCvZoomView(const string &name, cv::Mat &mat, int width, int height, int posX, int posY)
	{
		Name = name;
		cout << Name << endl;
		MatView = mat;

		cv::namedWindow(name, WINDOW_NORMAL);
		cv::imshow(name, mat);
		cv::resizeWindow(Name, width, height);
		cv::setMouseCallback(name, (cv::MouseCallback) DefaultMouseProcessor, this);

	}


	OpenCvZoomView::~OpenCvZoomView()
	{
	}

	void OpenCvZoomView::DefaultKeyboardProcessor()
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


	void OpenCvZoomView::ProcessEvents()
	{
		DefaultKeyboardProcessor();
		// randu(b, Scalar::all(0), Scalar::all(255));
	}


}