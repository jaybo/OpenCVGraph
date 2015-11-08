
#pragma once

#include "stdafx.h"

using namespace std;

namespace openCVGraph
{
	class  GraphData {
	public:
		std::string m_GraphName;			// Name of the loop processor running this graph
		bool abortOnESC;
		cv::Mat m_imCapture;
		cv::Mat m_imResult;
		std::vector<cv::Mat> m_imStack;
		int m_FrameNumber = 0;
        bool m_SettingsHaveChanged;
	};
}
