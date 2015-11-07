
#include "..\stdafx.h"
#include "CamXimea.h"

using namespace std;
using namespace cv;
namespace fs = ::boost::filesystem;

namespace openCVGui
{
#ifdef XIMEA_DIR
    CamXimea::CamXimea(std::string name, GraphData& graphData, bool showView, int width, int height, int x, int y)
        : CamDefault(name, graphData, showView, width, height, x, y)
    {
    }

    // keyWait required to make the UI activate
    bool CamXimea::processKeyboard(GraphData& data)
    {
        bool fOK = true;
        if (m_showView) {
            int c = waitKey(1);
            if (c != -1) {

                switch (c) {
                    // EXPOSURE
                case 0x00260000: // up arrow
                    Exposure(true);
                    break;
                case 0x00280000: // down arrow
                    Exposure(false);
                    break;

                    // GAIN
                case 0x00270000: // RIGHT arrow
                    Gain(true);
                    break;
                case 0x00250000: // LEFT arrow
                    Gain(false);
                    break;

                    // FOCUS
                case '.':
                    Focus(true);
                    break;
                case ',':
                    Focus(false);
                    break;
                case 27:    // ESC
                    fOK = false;
                    break;
                }

            }
        }
        else {
            return view.KeyboardProcessor();  // Hmm,  what to do here?
        }
        return fOK;
    }

    //Allocate resources if needed
    bool CamXimea::init(GraphData& graphData)
    {
        // call the base to read/write configs
        FrameProcessor::init(graphData);

        bool fOK = false;

        // CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA 
        if (camera_index.length() > 0) {

            std::stringstream(camera_index) >> cameraIndex;
            fOK = cap.open(cameraIndex);
            if (fOK) {
                // set camera specific properties
                if (camera_name == "Ximea16") {
#ifdef XIMEA_DIR
                    // only capture 3840x3840@16bpp
                    fOK = cap.set(CV_CAP_PROP_FRAME_WIDTH, 3840);
                    fOK = cap.set(CV_CAP_PROP_XI_OFFSET_X, 640);
                    fOK = cap.set(CV_CAP_PROP_XI_IMAGE_DATA_FORMAT, XI_MONO16);

                    // Limit the number of buffers
                    fOK = cap.set(CV_CAP_PROP_XI_BUFFERS_QUEUE_SIZE, (double)2);
                    fOK = cap.set(CV_CAP_PROP_XI_RECENT_FRAME, 1);

                    // Autogain off
                    fOK = cap.set(CV_CAP_PROP_XI_AEAG, 0);

                    // Enable aperature and focus
                    cap.set(CV_CAP_PROP_XI_LENS_MODE, 1);
                    m_focalDistance = cap.get(CV_CAP_PROP_XI_LENS_FOCUS_DISTANCE);
                    m_focalLength = cap.get(CV_CAP_PROP_XI_LENS_FOCAL_LENGTH);
                    m_aperatureValue = cap.get(CV_CAP_PROP_XI_LENS_APERTURE_VALUE);

                    fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE, 10);
                    m_focusMovementValue = cap.get(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE);

                    fOK = cap.set(CV_CAP_PROP_XI_EXPOSURE, 2000);
                    fOK = cap.set(CV_CAP_PROP_XI_GAIN, 1.0);
#endif
                }
                fOK = cap.read(graphData.imCapture);

                if (!graphData.imCapture.data)   // Check for invalid input
                {
                    fOK = false;
                    std::cout << "Could not open capture device #" << camera_index << std::endl;
                }
                else {
                    source = Camera;
                    fOK = true;
                }
            }
        }


        return fOK;
    }


    bool CamXimea::process(GraphData& graphData)
    {
        m_firstTime = false;
        bool fOK = true;

        switch (source) {
        case Camera:
            fOK = cap.read(graphData.imCapture);
            break;
        }

        if (m_showView && fOK) {
            if (camera_name == "Ximea16") {
                imView = 16 * graphData.imCapture;
            }
            else {
                imView = graphData.imCapture;
            }
            cv::imshow(m_CombinedName, imView);
        }
        return fOK;
    }

    // deallocate resources
    bool CamXimea::fini(GraphData& graphData)
    {
        if (cap.isOpened()) {
            cap.release();
        }
        return true;
    }



    void  CamXimea::saveConfig()
    {
        FileStorage fs2(m_persistFile, FileStorage::WRITE);
        fs2 << "tictoc" << tictoc;
        fs2 << "camera_index" << camera_index;
        fs2 << "camera_name" << camera_name;
        fs2 << "focus" << m_focalDistance;
        fs2 << "focalLength" << m_focalLength;
        fs2 << "aperature" << m_aperatureValue;
        fs2 << "focusMovementValue" << m_focusMovementValue;
        fs2 << "exposure" << m_exposure;
        fs2.release();
    }

    void  CamXimea::loadConfig()
    {
        FileStorage fs2(m_persistFile, FileStorage::READ);
        cout << m_persistFile << endl;

        fs2["tictoc"] >> tictoc;
        fs2["camera_index"] >> camera_index;
        fs2["camera_name"] >> camera_name;
        fs2["focus"] >> m_focalDistance;
        fs2["focalLength"] >> m_focalLength;
        fs2["aperature"] >> m_aperatureValue;
        fs2["focusMovementValue"] >> m_focusMovementValue;
        fs2["exposure"] >> m_exposure;
        fs2.release();
    }
#endif
}
