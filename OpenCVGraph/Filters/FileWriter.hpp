#pragma once

#ifndef INCLUDE_OCVG_FILEWRITER
#define INCLUDE_OCVG_FILEWRITER

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Simplest possible filter
    class FileWriter : public Filter
    {
    public:

        FileWriter::FileWriter(std::string name, GraphData& graphData,
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
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
            Filter::init(graphData);

            if (m_Enabled) {

                graphData.m_NeedCV_16UC1 = true;

                createDir(m_Directory);

                if (m_WriteOnKeyHit != "") {
                    m_WriteNextImage = false;
                }
            }
            return true;
        }

        // Do all of the work here.
        ProcessResult FileWriter::process(GraphData& graphData) override
        {
            if (m_WriteNextImage) {
                string fullName = m_Directory + '/' + m_BaseFileName + std::to_string(graphData.m_FrameNumber) + m_Ext;
                vector<int> params = { 
                    259,1,      // No compression, turn off LZW
                    279, 64     // Rows per strip (doesn't seem to affect perf - why not?)
                };
                imwrite(fullName, graphData.m_imOut16UC1, params);

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
            cvWriteComment((CvFileStorage *)*fs, "Set writeOnKeyHit to a single char to trigger write if that key is hit. Leave empty to write every frame.", 0);
            fs << "writeOnKeyHit" << m_WriteOnKeyHit.c_str();
        }

        void  FileWriter::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["directory"] >> m_Directory;
            fs["baseFileName"] >> m_BaseFileName;
            fs["ext"] >> m_Ext;
            fs["writeOnKeyHit"] >> m_WriteOnKeyHit;
        }

    private:
        string m_Directory = "C:/junk";
        string m_BaseFileName = "test";
        string m_Ext = ".tif";
        string m_WriteOnKeyHit = "";
        bool m_WriteNextImage = true;
    };
}
#endif