
#include "..\stdafx.h"
#include "CamXimea.h"

using namespace std;
using namespace cv;
namespace fs = ::boost::filesystem;

namespace openCVGraph
{

#ifdef XIMEA_DIR
    CamXimea::CamXimea(std::string name, GraphData& graphData, bool showView, int width, int height)
        : CamDefault(name, graphData, showView, width, height)
    {
    }

    // keyWait required to make the UI activate
    bool CamXimea::processKeyboard(GraphData& data, int key)
    {
        bool fOK = true;
        if (m_showView) {

            switch (key) {
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
        else {
            return view.KeyboardProcessor();  // Hmm,  what to do here?
        }
        return fOK;
    }

    //Allocate resources if needed
    bool CamXimea::init(GraphData& graphData)
    {
        // call the base to read/write configs
        Filter::init(graphData);

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
                fOK = cap.read(graphData.m_imCapture);

                if (!graphData.m_imCapture.data)   // Check for invalid input
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
            fOK = cap.read(graphData.m_imCapture);
            break;
        }

        if (m_showView && fOK) {
            if (camera_name == "Ximea16") {
                m_imView = 16 * graphData.m_imCapture;
            }
            else {
                m_imView = graphData.m_imCapture;
            }
            cv::imshow(m_CombinedName, m_imView);
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



    void  CamXimea::saveConfig(FileStorage& fs, GraphData& data)
    {
        fsf << "tictoc" << tictoc.c_str();
        fsf << "camera_index" << camera_index.c_str();
        fsf << "camera_name" << camera_name.c_str();
        fsf << "focus" << m_focalDistance;
        fsf << "focalLength" << m_focalLength;
        fsf << "aperature" << m_aperatureValue;
        fsf << "focusMovementValue" << m_focusMovementValue;
        fsf << "exposure" << m_exposure;
    }

    void  CamXimea::loadConfig(FileNode& fs, GraphData& data)
    {
        cout << m_persistFile << endl;

        fs["tictoc"] >> tictoc;
        fs["camera_index"] >> camera_index;
        fs["camera_name"] >> camera_name;
        fs["focus"] >> m_focalDistance;
        fs["focalLength"] >> m_focalLength;
        fs["aperature"] >> m_aperatureValue;
        fs["focusMovementValue"] >> m_focusMovementValue;
        fs["exposure"] >> m_exposure;
    }
#endif
}
