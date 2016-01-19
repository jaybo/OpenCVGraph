#pragma once

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // ------------------------------------------------
    // Write TIFF, PNG, or JPG depending on extension
    // ------------------------------------------------

    class FileWriter : public Filter
    {
    public:

        FileWriter::FileWriter(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool FileWriter::processKeyboard(GraphData& data, int key) override
        {
            bool fOK = true;
            char c[2];
            c[0] = char(key);
            c[1] = 0;
            if (string(c) == m_WriteOnKeyHit) {
                m_WriteNextImage = true;    // Trigger write
            }
            return fOK;
        }

        //Allocate resources if needed, and specify the image formats required
        bool FileWriter::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            bool fOK = Filter::init(graphData);

            createDir(m_Directory);

            if (m_WriteOnKeyHit != "") {
                m_WriteNextImage = false;
            }
            return fOK;
        }

        // Do all of the work here.
        ProcessResult FileWriter::process(GraphData& graphData) override
        {
            Mat src;

            switch (m_StreamIn) {
            case StreamIn::Corrected:
                src = graphData.m_CommonData->m_imCorrected;
                break;
            case StreamIn::CaptureRaw:
                src = graphData.m_CommonData->m_imCapture;
                break;
            case StreamIn::Out:
                src = graphData.m_imOut;
                break;
            }

            if (m_WriteNextImage && !src.empty()) {
                string fullName;
                if (m_UseSourceFileName && !graphData.m_CommonData->m_SourceFileName.empty()) {
                    auto s = graphData.m_CommonData->m_SourceFileName;
                    // strip off the path and add new directory
                    fullName = m_Directory + '/' + s.substr(s.find_last_of("\\/"), s.npos);
                }
                else {
                    if (!graphData.m_CommonData->m_DestinationFileName.empty()) {
                        fullName = graphData.m_CommonData->m_DestinationFileName;
                    }
                    else {
                        fullName = m_Directory + '/' + m_BaseFileName + std::to_string(graphData.m_FrameNumber) + m_Ext;
                    }
                }
                vector<int> params = { 
                    259,1,      // No compression, turn off LZW
                    279, 64     // Rows per strip (doesn't seem to affect perf - why not?)
                };
                imwrite(fullName, src, params);

                if (m_WriteOnKeyHit != "") {
                    m_WriteNextImage = false;
                }

            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void  FileWriter::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "directory" << m_Directory.c_str();
            fs << "baseFileName" << m_BaseFileName.c_str();
            fs << "ext" << m_Ext.c_str();
            fs << "useSourceFileName" << m_UseSourceFileName;
            cvWriteComment((CvFileStorage *)*fs, "Set writeOnKeyHit to a single char to trigger write if that key is hit. Leave empty to write every frame.", 0);
            fs << "writeOnKeyHit" << m_WriteOnKeyHit.c_str();
            cvWriteComment((CvFileStorage *)*fs, "writeCaptureSteam == 1 : write Cap stream, == 0 : write Out stream.", 0);
        }

        void  FileWriter::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["directory"] >> m_Directory;
            fs["baseFileName"] >> m_BaseFileName;
            fs["ext"] >> m_Ext;
            fs["useSourceFileName"] >> m_UseSourceFileName;
            fs["writeOnKeyHit"] >> m_WriteOnKeyHit;
        }

    private:
        string m_Directory = "C:/junk";
        string m_BaseFileName = "test";
        string m_Ext = ".tif";
        bool m_UseSourceFileName = false;
        string m_WriteOnKeyHit = "";
        bool m_WriteNextImage = true;
    };
}
