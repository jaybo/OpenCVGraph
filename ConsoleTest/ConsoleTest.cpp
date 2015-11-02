// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\OpenCVGui\FrameProcessor.h"
#include "..\OpenCVGui\LoopProcessor.h"
#include "..\OpenCVGui\OpenCvZoomView.h"
#include "..\OpenCVGui\FrameProcessors\FPImageSource.h"

using namespace cv;
using namespace std;
using namespace openCVGui;


int main()
{
	Mat a(200, 200, CV_16U);
	Mat b(200, 200, CV_8U);
	randu(b, Scalar::all(0), Scalar::all(255));
	randu(a, Scalar::all(0), Scalar::all(64000));


	LoopProcessor lp ("Loop1");

    FPImageSource fpImage("Image1", lp.gd, false);
    FrameProcessor fpAverage("average", lp.gd, false );

	lp.Processors.push_back(fpImage);
	lp.Processors.push_back(fpAverage);

    lp.GotoState(LoopProcessor::GraphState::Run);


	OpenCvZoomView view1("viewA", a, 1024, 1024, 100, 100);
	OpenCvZoomView view2("viewB", b, 300, 300, 400,400);

	bool fOK = true;
	while (fOK) {
		randu(a, Scalar::all(0), Scalar::all(64000));
		view1.ProcessEvents();
		view2.ProcessEvents();
	}
	return 0;
}

