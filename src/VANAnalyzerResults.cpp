#include "VanAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "VanAnalyzer.h"
#include "VanAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <sstream>

typedef union
{
    struct
    {
        uint8_t RTR : 1;
        uint8_t RNW : 1;
        uint8_t RAK : 1;
        uint8_t EXT : 1;
    }data;
    uint8_t Value;
}Command;


VanAnalyzerResults::VanAnalyzerResults( VanAnalyzer* analyzer, VanAnalyzerSettings* settings )
:	AnalyzerResults(),
    mSettings( settings ),
    mAnalyzer( analyzer )
{
}

VanAnalyzerResults::~VanAnalyzerResults()
{
}

void VanAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
    ClearResultStrings();
    Frame frame = GetFrame(frame_index);
    char number_str[128];
    std::stringstream ss;
    char command[4] = "-R-";
    char ack[2] = "N";

    switch (frame.mType)
    {
        case StartOfFrame:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
            ss << "SOF: " << number_str;
            AddResultString(ss.str().c_str());
            break;
        case IdentifierField:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 12, number_str, 128);
            ss << "IDEN: " << number_str;
            AddResultString(ss.str().c_str());
            break;
        case CommandField:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, number_str, 128);
            //EXT(1), RAK (0-no, 1-yes), RW (0-W,1-R), RTR (0-data, 1-no data)
            Command com;
            com.Value = frame.mData1;
            if (com.data.RAK == 1)
            {
                command[0] = 'A';
            }
            if (com.data.RNW == 0)
            {
                command[1] = 'W';
            }
            if (com.data.RTR == 0)
            {
                command[2] = 'D';
            }

            ss << "COM: " << number_str << "(" << command << ")";
            AddResultString(ss.str().c_str());
            break;
        case DataField:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
            ss << "DATA(" << frame.mData2 << "): " << number_str;
            AddResultString(ss.str().c_str());
            break;
        case FCSField:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
            ss << "FCS(" << frame.mData2 << "): " << number_str;
            AddResultString(ss.str().c_str());
            break;
        case EndOfData:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 2, number_str, 128);
            ss << "EOD: " << number_str;
            AddResultString(ss.str().c_str());
            break;
        case AckField:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 2, number_str, 128);
            if (frame.mData1 == 2)
            {
                ack[0] = 'A';
            }
            ss << "ACK: " << number_str << "(" << ack << ")";
            AddResultString(ss.str().c_str());
            break;
        case EndOfFrame:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
            ss << "EOF: " << number_str;
            AddResultString(ss.str().c_str());
            break;
        default:
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, number_str, 128);
            AddResultString(number_str);
            break;
    }

}

void VanAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
    //export_type_user_id is only important if we have more than one export type.
    std::stringstream ss;
    void* f = AnalyzerHelpers::StartFile(file);

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    U64 num_frames = GetNumFrames();
    U64 num_packets = GetNumPackets();

    for (U32 i = 0; i < num_packets; i++)
    {
        if (i != 0)
        {
            //below, we "continue" the loop rather than run to the end.  So we need to save to the file here.
            ss << std::endl;

            AnalyzerHelpers::AppendToFile((U8*)ss.str().c_str(), ss.str().length(), f);
            ss.str(std::string());


            if (UpdateExportProgressAndCheckForCancel(i, num_packets) == true)
            {
                AnalyzerHelpers::EndFile(f);
                return;
            }
        }

        U64 first_frame_id;
        U64 last_frame_id;
        GetFramesContainedInPacket(i, &first_frame_id, &last_frame_id);
        Frame frame = GetFrame(first_frame_id);

        char time_str[128];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

        ss << time_str << " Ch:0:";
        U64 frame_id = first_frame_id;

        for (U64 i = first_frame_id; i < last_frame_id; i++)
        {
            Frame frame = GetFrame(i);
            U8 numOfBits = 12;
            switch (frame.mType)
            {
                case StartOfFrame:
                    numOfBits = 12;
                case IdentifierField:
                    numOfBits = 12;
                case CommandField:
                    numOfBits = 4;
                case DataField:
                    numOfBits = 8;
                case FCSField:
                    numOfBits = 8;
                case AckField:
                    numOfBits = 2;
                default:
                    break;
            }

            if (frame.mType != StartOfFrame && frame.mType != EndOfData && frame.mType != AckField  && frame.mType != EndOfFrame)
            {
                char number_str[128];
                AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, numOfBits, number_str, 128);
                memmove(number_str, number_str + 2, strlen(number_str));//remove 0x from hex-string

                if (strlen(number_str) == 1 && frame.mType != CommandField)
                {
                    ss << " 0" << number_str;
                }
                else
                {
                    ss << " " << number_str;
                }
            }
            if (frame.mType == AckField)
            {
                if (frame.mData1 == 3)
                {
                    ss << " N";
                }
                else
                {
                    ss << " A";
                }
            }

        }
    }

    UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
    AnalyzerHelpers::EndFile(f);
}

void VanAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
    Frame frame = GetFrame( frame_index );
    ClearTabularText();

    char number_str[128];
    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
    AddTabularText( number_str );
#endif
}

void VanAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
    //not supported

}

void VanAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
    //not supported
}