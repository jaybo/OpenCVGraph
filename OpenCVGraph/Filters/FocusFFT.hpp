#pragma once

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Scores an image using its fft.  Omega is high frequency pixels to sum.  
    // Lowomegas are too effected by noise, and high omega values are too
    // effected by low - frequency effects.
    // See: http://www.csl.cornell.edu/~cbatten/pdfs/batten-image-processing-sem-ucthesis2000.pdf 


    class FocusFFT : public Filter , public ITemcaFocus
    {
    public:

        FocusFFT::FocusFFT(std::string name, GraphData& graphData,
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }

        bool FocusFFT::init(GraphData& graphData) override
        {
            Filter::init(graphData);
            graphData.m_CommonData->m_NeedCV_8UC1 = true;
            graphData.m_CommonData->m_NeedCV_16UC1 = true;

            if (m_Enabled) {
                if (m_showView) {
                    if (m_showViewControls) {
                        createTrackbar("omega", m_CombinedName, &m_Omega, m_DFTSize-1);
                        createTrackbar("view", m_CombinedName, (int*) &m_ImageIndex, PowerSpectrumROI);
                    }
                }
            }
            return true;
        }

        ProcessResult FocusFFT::process(GraphData& graphData) override
        {
            //if (graphData.m_UseCuda) {
            if (false) {
                //Scalar s;
                //graphData.m_imOutGpu16UC1 = graphData.m_CommonData->m_imCapGpu16UC1;
                //auto nPoints = graphData.m_CommonData->m_imCapGpu16UC1.size().area();

                //// X
                //m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_CommonData->m_imCapGpu16UC1.type(), graphData.m_imOutGpu16UC1.type(), 1, 0, m_kSize);
                //m_cudaFilter->apply(graphData.m_CommonData->m_imCapGpu16UC1, graphData.m_imOutGpu16UC1);
                //s = cv::cuda::sum(graphData.m_imOutGpu16UC1);
                //meanX = s[0] / nPoints;

                //// Y
                //m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_CommonData->m_imCapGpu16UC1.type(), graphData.m_imOutGpu16UC1.type(), 0, 1, m_kSize);
                //m_cudaFilter->apply(graphData.m_CommonData->m_imCapGpu16UC1, graphData.m_imOutGpu16UC1);
                //s = cv::cuda::sum(graphData.m_imOutGpu16UC1);
                //meanY = s[0] / nPoints;

                //meanXY = (meanX + meanY) / 2;
            }
            else {
                
                // http://docs.opencv.org/master/d8/d01/tutorial_discrete_fourier_transform.html#gsc.tab=0 

                if (graphData.m_CommonData->m_imCap16UC1.empty())
                {
                    graphData.m_CommonData->m_imCap16UC1 = graphData.m_CommonData->m_imCapture;
                }
                int w = graphData.m_CommonData->m_imCap16UC1.size().width;
                int h = graphData.m_CommonData->m_imCap16UC1.size().height;
                Rect rCropped = Rect(Point(w - m_DFTSize, h - m_DFTSize), Size(m_DFTSize, m_DFTSize));
                Mat IC = Mat(graphData.m_CommonData->m_imCap16UC1, rCropped);
                
                /*Mat I = Mat_<float>(IC);*/
                Mat I;
                IC.convertTo(I, CV_32FC1, 1/65536.f);

                Mat planes[] = { I, Mat::zeros(I.size(), CV_32F) };
                Mat complexI;   
                cv::merge(planes, 2, complexI);         // Add to the expanded another plane with zeros
                cv::dft(complexI, complexI);            // this way the result may fit in the source matrix

                // compute the magnitude and switch to logarithmic scale
                // => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
                split(complexI, planes);
                // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
                cv::magnitude(planes[0], planes[1], planes[0]);
                // planes[0] = magnitude
                Mat magI = planes[0];
                magI += Scalar::all(1);
                // switch to logarithmic scale
                cv::log(magI, magI);
                
                // crop the spectrum, if it has an odd number of rows or columns
                magI = magI(Rect(0, 0, magI.cols & -2, magI.rows & -2));
                // rearrange the quadrants of Fourier image  so that the origin is at the image center
                int cx = magI.cols/2;
                int cy = magI.rows/2;
                Mat q0(magI, Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant
                Mat q1(magI, Rect(cx, 0, cx, cy));  // Top-Right
                Mat q2(magI, Rect(0, cy, cx, cy));  // Bottom-Left
                Mat q3(magI, Rect(cx, cy, cx, cy)); // Bottom-Right
                Mat tmp;                           // swap quadrants (Top-Left with Bottom-Right)
                q0.copyTo(tmp);
                q3.copyTo(q0);
                tmp.copyTo(q3);
                q1.copyTo(tmp);                    // swap quadrant (Top-Right with Bottom-Left)
                q2.copyTo(q1);
                tmp.copyTo(q2);
                normalize(magI, magI, 0, 1, NORM_MINMAX); // Transform the matrix with float values into a
                // viewable image form (float between values 0 and 1).
                // imshow("Input Image"       , I   );
                // Show the result
                //imshow("spectrum magnitude", magI);

                m_PowerSpectrum = magI;

                // adaptive_filter in python
                cv::bilateralFilter(magI, tmp, 5, 50, 50);

                Mat polar;
                // polar transform
                cv::linearPolar(tmp, polar, Point (rCropped.width / 2, rCropped.height / 2), rCropped.width / 2, INTER_LINEAR);

                m_roiPowerSpectrum = polar(Range::all(), Range(1, rCropped.width - m_Omega));
                auto s = cv::mean(m_roiPowerSpectrum);
                m_FocusScore = s[0];
                 

            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void FocusFFT::processView(GraphData& graphData)  override
        {
            ClearOverlayText();

            switch (m_ImageIndex) {
            case ImageToView::PowerSpectrum:
                m_PowerSpectrum.convertTo(m_imView, CV_8UC1, 255.0);
                break;
            case ImageToView::PowerSpectrumROI:
                m_roiPowerSpectrum.convertTo(m_imView, CV_8UC1, 255.0);
                break;
            }

            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "FFT:  focus   astig  angle";
            DrawOverlayText(str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << std::setfill(' ') << std::fixed << std::setprecision(3)  << std::setw(10) << m_FocusScore << std::setw(8) << m_AstigmatismScore << std::setw(8) << m_AstigmatismAngle;
            DrawOverlayText(str.str(), Point(posLeft, 100), scale);

            Filter::processView(graphData);
        }

        void  FocusFFT::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            cvWriteComment((CvFileStorage *)*fs, "Power of 2 (256, 512, 1024, 2048)", 0);
            fs << "dft_size" << m_DFTSize;
            fs << "omega" << m_Omega;
            if (m_Omega >= m_DFTSize) {
                m_Omega = 0;
            }
        }

        void  FocusFFT::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["dft_size"] >> m_DFTSize;
            fs["omega"] >> m_Omega;
        }

        void FocusFFT::DFTSize(int dftSize) {
            m_DFTSize = dftSize;
        }

        FocusInfo getFocusInfo() { 
            FocusInfo fi; 
            fi.score = (float) m_FocusScore;
            fi.astigmatism = (float)m_AstigmatismAngle;
            fi.angle = (float) m_AstigmatismAngle;
            return fi;
        }

    private:

        enum ImageToView {
            PowerSpectrum,
            PowerSpectrumROI
        };

        ImageToView m_ImageIndex;           // which image to view
        int m_DFTSize = 512;
        int m_Omega = 50;           // number of high frequency components to consider
        double m_FocusScore;
        double m_AstigmatismScore;
        double m_AstigmatismAngle;

        cv::Ptr<cv::cuda::Filter> m_cudaFilter;

        Mat m_PowerSpectrum;
        Mat m_roiPowerSpectrum;
    };
}
