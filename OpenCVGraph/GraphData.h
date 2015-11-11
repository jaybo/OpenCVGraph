
#pragma once

#include "stdafx.h"

using namespace std;

namespace openCVGraph
{
	class  GraphData {
	public:
		std::string m_GraphName;			// Name of the loop processor running this graph
		bool m_AbortOnESC;                  // Exit the graph thread if ESC is hit?

		cv::Mat m_imCapture;                // Capture image.  Don't modify this except in capture filters.
        cv::Mat m_imCapture8U;               // 8 bit version
        cv::Mat m_imResult;                 // "Result" image.  Could be anything.  Capture filters just copy imCapture to imResult.

        // Cuda!
        cv::cuda::GpuMat m_imCaptureGpu16U;
        cv::cuda::GpuMat m_imCaptureGpu32F;
        cv::cuda::GpuMat m_imCaptureGpu8U;



		std::vector<cv::Mat> m_imStack;     // "Stack" of images used by cooperating filters.
		int m_FrameNumber = 0;              // Current frame being processed.
	};
}
