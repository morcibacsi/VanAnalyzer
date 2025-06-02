#include "VanAnalyzer.h"
#include "VanAnalyzerSettings.h"
#include "VanAnalyzerResults.h"
#include <AnalyzerChannelData.h>

VanAnalyzer::VanAnalyzer() : Analyzer2(), mSettings( new VanAnalyzerSettings() ), mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );

    // UseFrameV2();
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
    mNumSamplesIn8Bits = U32( mSamplesPerBit * 16.0 );

    WaitFor8RecessiveBits();

    mSerial->AdvanceToNextEdge();
    mFrameStart = true;

    NewByte();

    // line is low
    U64 startOfFrameSampleNumber = mSerial->GetSampleNumber();
    AddMarker( startOfFrameSampleNumber, AnalyzerResults::Start );

    while( 1 )
    {
        mEofFound = false;

        //--- Loop until the EOF (8 consecutive recessive bits)
        do
        {
            BitState currentBitState = mSerial->GetBitState();
            const U64 start = mSerial->GetSampleNumber();

            const U64 nextEdge = mSerial->GetSampleOfNextEdge();
            const U64 bitCount = ( nextEdge - start + mSamplesPerBit / 2 ) / mSamplesPerBit;

            if( mFrameStart )
            {
                mBitPos = 0;
                mbyteCount = 0;
                AddMarker( start, AnalyzerResults::Start );
            }

            // new byte
            if( mbitCount == 0 )
            {
                mStartOfFieldSampleNumber = start + mSamplesPerBit / 2;
            }

            // still in the message
            if( bitCount < 8 )
            {
                mFrameStart = false;
                for( U64 i = 0; i < bitCount; i++ )
                {
                    U64 bitPosition = start + mSamplesPerBit / 2 + i * mSamplesPerBit;
                    ProcessBit( currentBitState, bitPosition );
                }
            }
            else
            {
                // we found the EOF (8 consecutive recessive bits)
                if (mbyteCount == 2)
                {
                    // we have a query frame so print the identifier
                    AddFrame( mStartOfIdenFieldSampleNumber, mPreviousNonManchesterBitPosition + (mSamplesPerBit / 2)*2, (mIdent << 4) | (mByte >> 4), IdentifierField, mbyteCount - 2 );
                }
                else
                {
                    AddFrame( mStartOfFieldSampleNumber, mPreviousNonManchesterBitPosition + (mSamplesPerBit / 2)*2, mByte, DataField, mbyteCount - 2 );
                }
                
                AddMarker( mPreviousNonManchesterBitPosition + (mSamplesPerBit / 2)*2, AnalyzerResults::Stop );
                NewByte();
                mResults->CommitPacketAndStartNewPacket();
            }
            mSerial->AdvanceToNextEdge();
//
/*
            if( !mSerial->DoMoreTransitionsExistInCurrentData() )
            {
                AddFrame( mStartOfFieldSampleNumber, mPreviousNonManchesterBitPosition, mByte, DataField, mbyteCount - 2 );
                // end of stream
                AddMarker( start, AnalyzerResults::Stop );
                NewByte();
            }
//*/
        } while( mEofFound != true );
        //---
        mResults->CommitResults();
    }
}

void VanAnalyzer::ProcessBit( const BitState bitState, const U64 bitPosition )
{
    bool isManchesterBit = ( mbitCount + 1 ) % 5 == 0;

    mBitPositions[ mBitPos ] = bitPosition;

    if( mBitPos >= 5 )
    {
        mBitPos = 0;
    }
    else
    {
        mBitPos++;
    }

    if( !isManchesterBit )
    {
        if( bitState == mSettings->Recessive() )
        {
            mByte |= mask;
        }
        mask = mask >> 1;
        mPreviousNonManchesterBitPosition = bitPosition;
        AddMarker( bitPosition, AnalyzerResults::Dot );
    }
    else
    {
        // if we found the second manchester bit, then we have the full byte
        AddMarker( bitPosition, AnalyzerResults::X );
        if( mbitCount == 9 )
        {
            mbitCount = -1;
            mask = 1 << 7;
            switch( mbyteCount )
            {
            case 0:
            {
                mCOM = 0;
                mIdent = 0;
                AddFrame( mStartOfFieldSampleNumber, mPreviousNonManchesterBitPosition, mByte, StartOfFrame, 0 );
                break;
            }
            case 1:
            {
                mIdent = mByte;
                mStartOfIdenFieldSampleNumber = mStartOfFieldSampleNumber;

                break;
            }
            case 2:
            {
                mIdent = ( mIdent << 8 | mByte ) >> 4;
                mCOM = ( U8 )( mByte & 0xF );
                AddFrame( mStartOfIdenFieldSampleNumber, mBitPositions[ 0 ], mIdent, IdentifierField, 0 );
                AddFrame( mBitPositions[ 1 ], bitPosition, mCOM, CommandField, 0 );
                break;
            }
            default:
                AddFrame( mStartOfFieldSampleNumber, mPreviousNonManchesterBitPosition, mByte, DataField, mbyteCount - 2 );
                break;
            }

            mStartOfFieldSampleNumber = bitPosition;
            mByte = 0;
            mbyteCount++;
        }
    }

    mbitCount++;
}

void VanAnalyzer::NewByte()
{
    for (size_t i = 0; i < 6; i++)
    {
        mBitPositions[i] = 0;
    }
    
    mEofFound = true;
    mFrameStart = true;
    mbitCount = 0;
    mBitPos = 0;
    mbyteCount = 0;
    mByte = 0;
    mIdent = 0;
    mCOM = 0;
    mask = 1 << 7;
    mPreviousNonManchesterBitPosition = 0;
    mStartOfFieldSampleNumber = 0;
    mStartOfIdenFieldSampleNumber = 0;
}

bool VanAnalyzer::NeedsRerun()
{
    return false;
}

U32 VanAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                         SimulationChannelDescriptor** simulation_channels )
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
    if( mSerial->GetBitState() != mSettings->Recessive() )
        mSerial->AdvanceToNextEdge();

    for( ;; )
    {
        if( mSerial->WouldAdvancingCauseTransition( mNumSamplesIn8Bits ) == false )
            return;

        mSerial->AdvanceToNextEdge();
    }
}

void VanAnalyzer::AddMarker( const U64 inSampleNumber, const AnalyzerResults::MarkerType inMarker )
{
    mResults->AddMarker( inSampleNumber, inMarker, mSettings->mInputChannel );
}

// Adds a frame to the resultset
void VanAnalyzer::AddFrame( const U64 startingPoint, const U64 endingPoint, const U32 data, const U32 type, const U32 indexOfData )
{
    Frame frame;
    frame.mData1 = data;
    frame.mData2 = indexOfData;
    frame.mFlags = 0;
    frame.mType = type;
    frame.mStartingSampleInclusive = startingPoint;
    frame.mEndingSampleInclusive = endingPoint;

    mResults->AddFrame( frame );
//
/*
    FrameV2 frame_v2;

    // you can add any number of key value pairs. Each will get it's own column in the data table.
    if( type == IdentifierField )
    {
         frame_v2.AddInteger( "Identifier", frame.mData1 );
    }
    else if( type == CommandField )
    {
         frame_v2.AddByte( "Command", frame.mData1 );
         frame_v2.AddByte( "Data", frame.mData1 );
    }
    else
    {
         frame_v2.AddByte( "Data", frame.mData1 );
    }
    // The second parameter is the frame "type". Any string is allowed.
    mResults->AddFrameV2( frame_v2, VanFrameTypeForDisplay[ type ], frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );
//*/

    mResults->CommitResults();
    ReportProgress( frame.mEndingSampleInclusive );
}
