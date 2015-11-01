// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\OpenCVGui\FrameProcessor.h"
#include "..\OpenCVGui\FrameLoopProcessor.h"
#include "..\OpenCVGui\OpenCvZoomView.h"

using namespace cv;
using namespace std;
using namespace openCVGui;

#ifdef _DEBUG               
#pragma comment(lib, "opencv_core300d.lib")       
#pragma comment(lib, "opencv_highgui300d.lib")    
#pragma comment(lib, "opencv_imgcodecs300d.lib")  
//#pragma comment(lib, "opencv_text300d.lib")  
#pragma comment(lib, "opencv_features2d300d.lib")  
#pragma comment(lib, "opencv_imgproc300d.lib")  

#else       
#pragma comment(lib, "opencv_core300.lib")       
#pragma comment(lib, "opencv_highgui300.lib")    
#pragma comment(lib, "opencv_imgcodecs300.lib")    
//#pragma comment(lib, "opencv_text300.lib")  
#pragma comment(lib, "opencv_features2d300.lib")  
#pragma comment(lib, "opencv_imgproc300d.lib")  
#endif    



int main()
{
	Mat a(200, 200, CV_16U);
	Mat b(200, 200, CV_8U);
	randu(b, Scalar::all(0), Scalar::all(255));
	
	GraphData *gd = new GraphData();

	FrameLoopProcessor *flp = new FrameLoopProcessor("Loop1");

	auto foo = new FrameProcessor("average", *gd, false );
	auto view1 = new OpenCvZoomView("viewA", a, 200, 200, 100, 100);
	auto view2 = new OpenCvZoomView("viewB", b, 300, 300, 400,400);

	while (true) {
		randu(b, Scalar::all(0), Scalar::all(255));
		view1->ProcessEvents();
		view2->ProcessEvents();
	}
	return 0;
}

