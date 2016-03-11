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
                        createTrackbar("FreqStart", m_CombinedName, &m_FreqStart, m_DFTSize - 1);
                        createTrackbar("FreqEnd", m_CombinedName, &m_FreqEnd, m_DFTSize-1);
                        createTrackbar("view", m_CombinedName, (int*) &m_ImageIndex, PowerSpectrumROI);
                    }
                }
            }
            return true;
        }

        ProcessResult FocusFFT::process(GraphData& graphData) override
        {
            // prevent change FFT size or min max freq while processing
            std::lock_guard < std::mutex > lock(m_mutex);

            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_16UC1, false);
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, false);

            EnforceFreqLimits();

            int w = graphData.m_CommonData->m_imCapture.size().width;
            int h = graphData.m_CommonData->m_imCapture.size().height;
            Rect rCropped = Rect(Point(w/2 - m_DFTSize/2, h/2 - m_DFTSize/2), Size(m_DFTSize, m_DFTSize));

             if (graphData.m_UseCuda) {
            //if (false) {
#ifdef WITH_CUDA
                cv::cuda::Stream stream;

                cuda::GpuMat IC = cuda::GpuMat(graphData.m_CommonData->m_imCaptureGpu16UC1, rCropped);

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

                cuda::normalize(magI, magI, 0, 1, NORM_MINMAX, -1); 
                m_PowerSpectrumGpu = magI;

                // adaptive_filter in python
                cuda::bilateralFilter(magI, tmp, 5, 50, 50);

#if USE_FUNCTIONING_NON_GPU_VERSION_OF_POLAR_MAPPING
                // polar transform
                Mat tmpCpu;
                tmp.download(tmpCpu);
                Mat polar;
                
                cv::linearPolar(tmpCpu, polar, Point(rCropped.width / 2, rCropped.height / 2), rCropped.width / 2, INTER_LINEAR);

                m_RoiPowerSpectrum = polar(Range::all(), Range(m_FreqStart, m_FreqEnd));
                auto s = cv::mean(m_RoiPowerSpectrum);
                m_FocusScore = s[0];
#else
                int nX = m_FreqEnd - m_FreqStart;
                if (nX != m_AstigWidth || m_RecalcLUTs)
                {
                    m_AstigWidth = nX;
                    m_RecalcLUTs = false;
                    // Create a 2D lookup table which basically does the same thing as linearPolar in CPU land
                    int sx = rCropped.width / 2;
                    int sy = rCropped.height / 2;

                    // X: 0.. m_AstigWidth, Y: 0.. ASTIGMATISM_SIZE
                    float *xIndex = new float [ASTIGMATISM_SIZE *m_AstigWidth];
                    float *yIndex = new float[ASTIGMATISM_SIZE * m_AstigWidth];
                    for (int y = 0; y < ASTIGMATISM_SIZE; y++) {
                        auto a = M_PI * 2 * y / ASTIGMATISM_SIZE;
                        for (int x = 0; x < m_AstigWidth; x++) {
                            auto x0 = cos(a) * (x + m_FreqStart) + sx;
                            auto y0 = sin(a) * (x + m_FreqStart) + sy;
                            xIndex[y * m_AstigWidth + x] = (float)x0;
                            yIndex[y * m_AstigWidth + x] = (float)y0;
                        }
                    }
                    Mat imXMap = Mat(ASTIGMATISM_SIZE, m_AstigWidth, CV_32F, xIndex);
                    Mat imYMap = Mat(ASTIGMATISM_SIZE, m_AstigWidth, CV_32F, yIndex);

                    m_outXLUTGpu.upload(imXMap);
                    m_outYLUTGpu.upload(imYMap);

                    delete xIndex;
                    delete yIndex;
                }
                // Create the polar version via lookup table
                cuda::remap(tmp, m_RoiPowerSpectrumGpu, m_outXLUTGpu, m_outYLUTGpu, INTER_NEAREST, BORDER_REFLECT101);

                cuda::reduce(m_RoiPowerSpectrumGpu, m_imOutProfileGpu, 1, CV_REDUCE_AVG);
                cv::Scalar sum = cuda::absSum(m_imOutProfileGpu);
                m_imOutProfileGpu.download(m_imOutProfile);
                //auto s = cv::sum(m_imOutProfile);
                //m_FocusScore = s[0];
                m_FocusScore = sum[0];


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

                m_RoiPowerSpectrum = polar(Range::all(), Range(m_FreqStart, m_FreqEnd));
                auto s = cv::mean(m_RoiPowerSpectrum);
                m_FocusScore = s[0];

            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        // Limit the start and end frequencies to valid ranges (non-overlapping)
        void FocusFFT::EnforceFreqLimits()
        {
            if (m_FreqEnd >= m_DFTSize) {
                m_FreqEnd = m_DFTSize - 1;
            }
            if (m_FreqStart >= m_FreqEnd) {
                m_FreqStart = m_FreqEnd - 1;
            }
            if (m_FreqStart < 0) {
                m_FreqStart = 0;
            }
            if (m_FreqEnd <= m_FreqStart) {
                m_FreqEnd = m_FreqStart + 1;
            }
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
                if (graphData.m_UseCuda) {
                    m_RoiPowerSpectrumGpu.download(m_RoiPowerSpectrum);
                }
                m_RoiPowerSpectrum.convertTo(m_imView, CV_8UC1, 255.0);
                break;
            }

            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "FFT:  focus";
            DrawOverlayText(str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << std::setfill(' ') << std::fixed << std::setprecision(3)  << std::setw(10) << m_FocusScore;
            DrawOverlayText(str.str(), Point(posLeft, 100), scale);

            Filter::processView(graphData);
        }

        void  FocusFFT::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            cvWriteComment((CvFileStorage *)*fs, "Power of 2 (256, 512, 1024, 2048)", 0);
            fs << "dft_size" << m_DFTSize;
            fs << "freq_start" << m_FreqStart;
            fs << "freq_end" << m_FreqEnd;
        }

        void  FocusFFT::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["dft_size"] >> m_DFTSize;
            fs["freq_start"] >> m_FreqStart;
            fs["freq_end"] >> m_FreqEnd;

            EnforceFreqLimits();
        }

        void FocusFFT::DFTSize(int dftSize) {
            m_DFTSize = dftSize;
            m_RecalcLUTs = true;
        }

        void setFFTSize(int dimension, int startFreq, int endFreq) {
            std::lock_guard < std::mutex > lock(m_mutex);

            DFTSize(dimension);
            m_FreqStart = startFreq;
            m_FreqEnd = endFreq;
        }

        FocusInfo getFocusInfo() { 
            FocusInfo fi; 
            fi.focus_score = (float) m_FocusScore;
            fi.astig_score = 0.0;   // calculated in Python world for now
            fi.astig_angle = 0.0;   // calculated in Python world for now
            memcpy(fi.astig_profile, m_imOutProfile.data, sizeof(float) * ASTIGMATISM_SIZE);
            return fi;
        }

    private:
        std::mutex m_mutex;

        enum ImageToView {
            PowerSpectrum,
            PowerSpectrumROI
        };

        ImageToView m_ImageIndex;           // which image to view
        int m_DFTSize = 256;
        // Focus will use all DFT components between m_FreqStart and m_FreqEnd
        int m_FreqStart = 50;
        int m_FreqEnd = m_DFTSize - 5;           

        double m_FocusScore;
        double m_AstigmatismScore;
        double m_AstigmatismAngle;
#ifdef WITH_CUDA
        cv::Ptr<cv::cuda::Filter> m_cudaFilter;
        cuda::GpuMat m_outXLUTGpu, m_outYLUTGpu;
        cuda::GpuMat m_imOutProfileGpu; 
        cuda::GpuMat m_PowerSpectrumGpu;
        cuda::GpuMat m_RoiPowerSpectrumGpu;
        Mat m_imOutProfile;
#endif
        Mat m_PowerSpectrum;
        Mat m_RoiPowerSpectrum;
        int m_AstigWidth = -1;
        bool m_RecalcLUTs = true;
    };
}
