#pragma once

#include "..\stdafx.h"

using namespace cv;
using namespace cuda;
using namespace std;

namespace openCVGraph
{
    // Scores an image using its fft.  Omega is high frequency pixels to sum.  
    // Lowomegas are too effected by noise, and high omega values are too
    // effected by low - frequency effects.
    // See: http://www.csl.cornell.edu/~cbatten/pdfs/batten-image-processing-sem-ucthesis2000.pdf 

#define ASTIGMATISM_SIZE 360

    class FocusFFT : public Filter , public ITemcaFocus
    {
    public:

        FocusFFT::FocusFFT(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool FocusFFT::init(GraphData& graphData) override
        {
            Filter::init(graphData);

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
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_16UC1, false);
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, false);

            int w = graphData.m_CommonData->m_imCapture.size().width;
            int h = graphData.m_CommonData->m_imCapture.size().height;
            Rect rCropped = Rect(Point(w/2 - m_DFTSize/2, h/2 - m_DFTSize/2), Size(m_DFTSize, m_DFTSize));

             if (graphData.m_UseCuda) {
            //if (false) {
#ifdef WITH_CUDA
                cuda::GpuMat IC = cuda::GpuMat(graphData.m_CommonData->m_imCaptureGpu16UC1, rCropped);

                /*Mat I = Mat_<float>(IC);*/
                cuda::GpuMat I;
                IC.convertTo(I, CV_32FC1, 1 / 65536.f);
                GpuMat C = GpuMat(I.size(), CV_32F);
                C.setTo(0);
                GpuMat planes[] = { I, C };
                GpuMat complexI;
                cuda::merge(planes, 2, complexI);                               // Add to the expanded another plane with zeros
                cuda::dft(complexI, complexI, Size(m_DFTSize, m_DFTSize));      // this way the result may fit in the source matrix

                                                                                // compute the magnitude and switch to logarithmic scale
                                                                                // => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
                cuda::split(complexI, planes);
                // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
                cuda::magnitude(planes[0], planes[1], planes[0]);
                // planes[0] = magnitude
                GpuMat magI = planes[0];
                cuda::add (magI, Scalar(1), magI);
                // switch to logarithmic scale
                cuda::log(magI, magI);

                // crop the spectrum, if it has an odd number of rows or columns
                magI = magI(Rect(0, 0, magI.cols & -2, magI.rows & -2));

                // rearrange the quadrants of Fourier image  so that the origin is at the image center
                int cx = magI.cols / 2;
                int cy = magI.rows / 2;
                GpuMat q0(magI, Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant
                GpuMat q1(magI, Rect(cx, 0, cx, cy));  // Top-Right
                GpuMat q2(magI, Rect(0, cy, cx, cy));  // Bottom-Left
                GpuMat q3(magI, Rect(cx, cy, cx, cy)); // Bottom-Right
                GpuMat tmp;                           // swap quadrants (Top-Left with Bottom-Right)
                q0.copyTo(tmp);
                q3.copyTo(q0);
                tmp.copyTo(q3);
                q1.copyTo(tmp);                    // swap quadrant (Top-Right with Bottom-Left)
                q2.copyTo(q1);
                tmp.copyTo(q2);

                cuda::normalize(magI, magI, 0, 1, NORM_MINMAX, -1); // Transform the matrix with float values into a
                m_PowerSpectrumGpu = magI;

                // adaptive_filter in python
                cuda::bilateralFilter(magI, tmp, 5, 50, 50);
#if true
                // polar transform
                Mat tmpCpu;
                tmp.download(tmpCpu);
                Mat polar;
                
                cv::linearPolar(tmpCpu, polar, Point(rCropped.width / 2, rCropped.height / 2), rCropped.width / 2, INTER_LINEAR);

                m_RoiPowerSpectrum = polar(Range::all(), Range(1, rCropped.width - m_Omega));
                auto s = cv::mean(m_RoiPowerSpectrum);
                m_FocusScore = s[0];
#else
                int nX = rCropped.width - m_Omega;
                if (nX != m_AstigWidth)
                {
                    m_AstigWidth = nX;
                    // Create a 2D lookup table which basically does the same thing as linearPolar in CPU land

                    // X: 0.. m_AstigWidth, Y: 0.. ASTIGMATISM_SIZE
                    float *rowIndex = new float [ASTIGMATISM_SIZE *m_AstigWidth];
                    float *angIndex = new float[ASTIGMATISM_SIZE * m_AstigWidth];
                    for (int i = 0; i < ASTIGMATISM_SIZE; i++) {
                        for (int j = 0; j < m_AstigWidth; j++) {
                            rowIndex[i * ASTIGMATISM_SIZE + j] = j;
                            angIndex[i * ASTIGMATISM_SIZE + j] = 360.0 / (i / (float) ASTIGMATISM_SIZE);  // all rows have same angle
                        }
                    }
                    Mat imRowIndex = Mat(ASTIGMATISM_SIZE, m_AstigWidth, CV_32F, rowIndex);
                    Mat imAngIndex = Mat(ASTIGMATISM_SIZE, m_AstigWidth, CV_32F, angIndex);
                    GpuMat rangeX(imRowIndex);
                    GpuMat rangeY(imAngIndex);
                    delete rowIndex;
                    delete angIndex;

                    // angle
                    GpuMat outXGpu, outYGpu;
                    Mat outX, outY;
                    cuda::polarToCart(rangeX, rangeY, outXGpu, outYGpu, true);
                    outXGpu.download(outX);
                    outYGpu.download(outY);
                }

#endif

#else
                assert(0);
#endif
            }
            else {
                
                // http://docs.opencv.org/master/d8/d01/tutorial_discrete_fourier_transform.html#gsc.tab=0 

                if (graphData.m_CommonData->m_imCapture16UC1.empty())
                {
                    graphData.m_CommonData->m_imCapture16UC1 = graphData.m_CommonData->m_imCapture;
                }

                Mat IC = Mat(graphData.m_CommonData->m_imCapture16UC1, rCropped);
                
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
                m_PowerSpectrum = magI;

                // adaptive_filter in python
                cv::bilateralFilter(magI, tmp, 5, 50, 50);

                // polar transform
                Mat polar;
                cv::linearPolar(tmp, polar, Point (rCropped.width / 2, rCropped.height / 2), rCropped.width / 2, INTER_LINEAR);

                m_RoiPowerSpectrum = polar(Range::all(), Range(1, rCropped.width - m_Omega));
                auto s = cv::mean(m_RoiPowerSpectrum);
                m_FocusScore = s[0];

            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void FocusFFT::processView(GraphData& graphData)  override
        {
            ClearOverlayText();

            switch (m_ImageIndex) {
            case ImageToView::PowerSpectrum:
                if (graphData.m_UseCuda) {
                    m_PowerSpectrumGpu.download(m_PowerSpectrum);
                }
                m_PowerSpectrum.convertTo(m_imView, CV_8UC1, 255.0);
                break;
            case ImageToView::PowerSpectrumROI:
                m_RoiPowerSpectrum.convertTo(m_imView, CV_8UC1, 255.0);
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
        int m_DFTSize = 256;
        int m_Omega = 50;           // number of high frequency components to consider
        double m_FocusScore;
        double m_AstigmatismScore;
        double m_AstigmatismAngle;
#ifdef WITH_CUDA
        cv::Ptr<cv::cuda::Filter> m_cudaFilter;
#endif
        Mat m_PowerSpectrum;
        Mat m_RoiPowerSpectrum;
        cuda::GpuMat m_PowerSpectrumGpu;
        cuda::GpuMat m_RoiPowerSpectrumGpu;    
        int m_AstigWidth = -1;
    };
}
