// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// Boost
#include <boost/thread.hpp>

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


	LoopProcessor lp1 ("Loop1");



	std::shared_ptr<FPImageSource> fpImage1(new FPImageSource("Image1", lp1.gd, true));
	// std::shared_ptr<FrameProcessor> fpAverage(new FPImageSource("average", lp.gd, false));
	std::shared_ptr<FPImageSource> fpImage2 (new FPImageSource("Image2", lp1.gd, true));

	lp1.Processors.push_back(fpImage1);
	// lp.Processors.push_back(fpAverage);
	lp1.Processors.push_back(fpImage2);

	lp1.StartThread();
    lp1.GotoState(LoopProcessor::GraphState::Run);


	/*OpenCvZoomView view1("viewA", a, 1024, 1024, 100, 100);
	OpenCvZoomView view2("viewB", b, 300, 300, 400,400);

	bool fOK = true;
	while (fOK) {
		randu(a, Scalar::all(0), Scalar::all(64000));
		view1.ProcessEvents();
		view2.ProcessEvents();
	}*/

	lp1.JoinThread();
	return 0;
}

