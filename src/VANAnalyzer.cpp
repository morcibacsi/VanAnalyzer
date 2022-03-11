#include "VanAnalyzer.h"
#include "VanAnalyzerSettings.h"
#include "VanAnalyzerResults.h"
#include <AnalyzerChannelData.h>

VanAnalyzer::VanAnalyzer()
:	Analyzer2(),  
    mSettings( new VanAnalyzerSettings() ),
    mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );
}

VanAnalyzer::~VanAnalyzer()
{
    KillThread();
}

void VanAnalyzer::SetupResults()
{
    mResults.reset( new VanAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void VanAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();

    mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

    mSamplesPerBit = mSampleRateHz / mSettings->mBitRate;
    mNumSamplesIn8Bits = U32(mSamplesPerBit * 6.0);

    WaitFor8RecessiveBits();

    for( ; ; )
    {

        mSerial->AdvanceToNextEdge(); //falling edge -- beginning of the start bit

        U64 sampleNumberToReturnAsFrameStart = mSerial->GetSampleNumber();
        U64 sampleNumberToReturnAsFrameEnding = mSerial->GetSampleNumber();

        mResults->AddMarker(sampleNumberToReturnAsFrameStart, AnalyzerResults::Start, mSettings->mInputChannel);

        #pragma region StartOfFrame

        U8 sof1;
        U8 sof2;

        GetNibble(sof1, sampleNumberToReturnAsFrameEnding);
        GetNibble(sof2, sampleNumberToReturnAsFrameEnding);
        mStartOfFrame = (sof1 << 4) | sof2;

        AddFrame(sampleNumberToReturnAsFrameStart, mSerial->GetSampleNumber(), mStartOfFrame, StartOfFrame, 0);

        #pragma endregion

        #pragma region IdentifierField

        sampleNumberToReturnAsFrameStart = mSerial->GetSampleNumber();

        U8 ident1;
        U8 ident2;
        U8 ident3;
        GetNibble(ident1, sampleNumberToReturnAsFrameEnding);
        GetNibble(ident2, sampleNumberToReturnAsFrameEnding);
        GetNibble(ident3, sampleNumberToReturnAsFrameEnding);

        mIdentifier = (ident1 << 8) | (ident2 << 4) | ident3;

        AddFrame(sampleNumberToReturnAsFrameStart, sampleNumberToReturnAsFrameEnding, mIdentifier, IdentifierField, 0);

        #pragma endregion

        #pragma region CommandField

        sampleNumberToReturnAsFrameStart = mSerial->GetSampleNumber();

        U8 command;
        GetNibble(command, sampleNumberToReturnAsFrameEnding);
        AddFrame(sampleNumberToReturnAsFrameStart, sampleNumberToReturnAsFrameEnding, command, CommandField, 0);

        #pragma endregion

        #pragma region DataField

        U8 data;
        U8 counter = 0;
        bool endOfData = false;

        Frame arr[30];

        do
        {
            sampleNumberToReturnAsFrameStart = mSerial->GetSampleNumber();
            U8 data1;
            U8 data2;

            GetNibble(data1, sampleNumberToReturnAsFrameEnding);
            endOfData = GetNibble(data2, sampleNumberToReturnAsFrameEnding);

            data = (data1 << 4) | data2;

            AddFrame(sampleNumberToReturnAsFrameStart, sampleNumberToReturnAsFrameEnding, data, DataField, counter);

            counter++;
            if (counter == 30)
            {
                break;
            }
        } while (!endOfData);

        #pragma endregion

        #pragma region EndOfData

        //end of data is kind of hacky because how the protocol works
        //we do not know how many data bytes we have to read so in order to have a nice looking label we are returning from the GetNibble() method but the first dot in this frame is actually drawn by that method
        //thats why we cannot use the Get2Bits() method to get the value
        //maybe a better GetNibble() method could be written...
        data = 0;
        U8 mask = 1 << 1;

        if (mSerial->GetBitState() == mSettings->Recessive())
        {
            data |= mask;
        }
        mask = mask >> 1;


        sampleNumberToReturnAsFrameStart = sampleNumberToReturnAsFrameEnding;
        mSerial->Advance(mSamplesPerBit);

        if (mSerial->GetBitState() == mSettings->Recessive())
        {
            data |= mask;
        }
        mask = mask >> 1;

        mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
        // after the end of data there should be an edge with the ACK and EOF part, so we sync to that edge
        mSerial->AdvanceToNextEdge();
        sampleNumberToReturnAsFrameEnding = mSerial->GetSampleNumber();
        AddFrame(sampleNumberToReturnAsFrameStart, sampleNumberToReturnAsFrameEnding, data, EndOfData, 0);


        #pragma endregion

        #pragma region AckField

        sampleNumberToReturnAsFrameStart = sampleNumberToReturnAsFrameEnding;
        data = Get2Bits(sampleNumberToReturnAsFrameEnding);

        AddFrame(sampleNumberToReturnAsFrameStart, sampleNumberToReturnAsFrameEnding, data, AckField, 0);

        #pragma endregion

        #pragma region EndOfFrame

        sampleNumberToReturnAsFrameStart = sampleNumberToReturnAsFrameEnding;

        data = GetByte(sampleNumberToReturnAsFrameEnding);

        AddFrame(sampleNumberToReturnAsFrameStart, sampleNumberToReturnAsFrameEnding, data, EndOfFrame, 0);

        #pragma endregion

        mResults->AddMarker(sampleNumberToReturnAsFrameEnding, AnalyzerResults::Stop, mSettings->mInputChannel);

        mResults->CommitPacketAndStartNewPacket();
    }
}

bool VanAnalyzer::NeedsRerun()
{
    return false;
}

U32 VanAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitilized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 VanAnalyzer::GetMinimumSampleRateHz()
{
    return mSettings->mBitRate * 4;
}

const char* VanAnalyzer::GetAnalyzerName() const
{
    return "VAN";
}

const char* GetAnalyzerName()
{
    return "VAN";
}

Analyzer* CreateAnalyzer()
{
    return new VanAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}

void VanAnalyzer::WaitFor8RecessiveBits()
{
    if (mSerial->GetBitState() == mSettings->Recessive())
        mSerial->AdvanceToNextEdge();

    for (; ; )
    {
        if (mSerial->WouldAdvancingCauseTransition(mNumSamplesIn8Bits) == false)
            return;

        mSerial->AdvanceToNextEdge();
    }
}

// Adds a frame to the resultset
void VanAnalyzer::AddFrame(U64 startingPoint, U64 endingpoint, U32 data1, U32 type, U32 data2)
{
    Frame frame;
    frame.mData1 = data1;
    frame.mData2 = data2;
    frame.mFlags = 0;
    frame.mType = type;
    frame.mStartingSampleInclusive = startingPoint;
    frame.mEndingSampleInclusive = endingpoint;

    mResults->AddFrame(frame);
    mResults->CommitResults();
    ReportProgress(frame.mEndingSampleInclusive);
}

// Gets half a byte data from the stream
// The method also adjusts the position in the stream as we are reading the data from it
// It is a kind of a hacky solution due to the nature of the protocol
// We are handling the end of data bytes detection here also
bool VanAnalyzer::GetNibble(U8& nibbleRead, U64& sampleNumberToReturnAsFrameEnding)
{
    bool didWeReachEndOfData = false;
    mSerial->Advance((mSamplesPerBit / 2));

    nibbleRead = 0;
    U8 mask = 1 << 3;
    for (U32 i = 0; i<5; i++)
    {
        if (i != 4)
        {
            //we are placing a DOT marker to every non E-Manchester bit and also save the state of the signal
            mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
            if (mSerial->GetBitState() == mSettings->Recessive())
            {
                nibbleRead |= mask;
            }
            mask = mask >> 1;
        }

        if (i == 3)
        {
            //this is needed because of the last data byte
            //if we are at the 4th byte we check if we have an E-Manchester violation and if we have that means we reached the last byte so we should return
            if (!mSerial->WouldAdvancingCauseTransition(mSamplesPerBit))
            {
                didWeReachEndOfData = true;
                sampleNumberToReturnAsFrameEnding = mSerial->GetSampleNumber() - (mSamplesPerBit / 2);

                return didWeReachEndOfData;
            }

            //otherwise if it is a normal data byte that means we have to synchronize
            //every 5th bit is a synchronization bit so before them we are syncing to the edge of the signal (the array starts at 0 so thats why we have a smaller number by 1)
            mSerial->AdvanceToNextEdge();
            mSerial->Advance((mSamplesPerBit / 2));
            mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::X, mSettings->mInputChannel);
        }
        else if (i != 4)
        {
            //if we are not at a sync-bit we move ahead by 1 TS
            mSerial->Advance(mSamplesPerBit);
        }
        if (i == 4)
        {
            //when we reach the last measurement point we move ahead by half of a TS to have a nice looking boundary on the interface
            mSerial->Advance((mSamplesPerBit / 2));
        }
    }
    sampleNumberToReturnAsFrameEnding = mSerial->GetSampleNumber();
    return didWeReachEndOfData;
}

U8 VanAnalyzer::GetByte(U64& sampleNumberToReturnAsFrameEnding)
{
    U8 byteRead = 0;
    U8 mask = 1 << 7;
    
    mSerial->Advance((mSamplesPerBit / 2));

    for (U32 i = 0; i<8; i++)
    {
        mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
        if (mSerial->GetBitState() == mSettings->Recessive())
        {
            byteRead |= mask;
        }
        mask = mask >> 1;

        if (i != 7)
        {
            mSerial->Advance(mSamplesPerBit);
        }
    }
    sampleNumberToReturnAsFrameEnding = mSerial->GetSampleNumber();

    return byteRead;
}

U8 VanAnalyzer::Get2Bits(U64& sampleNumberToReturnAsFrameEnding)
{
    U8 byteRead = 0;
    U8 mask = 2;

    mSerial->Advance((mSamplesPerBit / 2));

    for (U32 i = 0; i<2; i++)
    {
        mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
        if (mSerial->GetBitState() == mSettings->Recessive())
        {
            byteRead |= mask;
        }
        mask = mask >> 1;

        if (i != 1)
        {
            mSerial->Advance(mSamplesPerBit);
        }

        if (i == 1)
        {
            mSerial->Advance(mSamplesPerBit / 2);
        }
    }
    sampleNumberToReturnAsFrameEnding = mSerial->GetSampleNumber();

    return byteRead;
}
