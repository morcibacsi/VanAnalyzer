#ifndef VAN_ANALYZER_H
#define VAN_ANALYZER_H

#include <Analyzer.h>
#include "VanAnalyzerResults.h"
#include "VanSimulationDataGenerator.h"

class VanAnalyzerSettings;
class ANALYZER_EXPORT VanAnalyzer : public Analyzer2
{
  private:
    char* VanFrameTypeForDisplay[ 9 ] = { "SOF", "IDENT", "COM", "DATA", "FCS", "EOD", "ACK", "EOF", "ERROR" };

  public:
    VanAnalyzer();
    virtual ~VanAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

  protected: // vars
    std::auto_ptr<VanAnalyzerSettings> mSettings;
    std::auto_ptr<VanAnalyzerResults> mResults;
    AnalyzerChannelData* mSerial;

    U64 mBitPositions[ 6 ] = { 0 }; // used to collect the bit positions so we can draw a good looking IDENT and COM field
    U8 mBitPos = 0;                 // indexer for the mBitPositions array

    U64 mStartOfFieldSampleNumber = 0;
    U64 mStartOfIdenFieldSampleNumber = 0;
    U64 mPreviousNonManchesterBitPosition = 0;

    U8 mbitCount = 0;  // used to determine if a bit is Manchester bit
    U8 mbyteCount = 0; // used to determine which part of the message we are processing
    U8 mByte = 0;      // we are building the VAN byte in this variable bit by bit
    U8 mask = 1 << 7;

    bool mFrameStart = true;
    bool mEofFound = false;

    VanSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    void WaitFor8RecessiveBits();
    void AddFrame( const U64 startingPoint, const U64 endingPoint, const U32 data, const U32 type, const U32 indexOfData );
    void ProcessBit( const BitState bitState, const U64 bitPosition );

    void AddMarker( const U64 inSampleNumber, const AnalyzerResults::MarkerType inMarker );
    void NewByte();

    // VAN analysis vars:
    U32 mSampleRateHz;
    U32 mSamplesPerBit;

    // VAN vars
    U32 mNumSamplesIn8Bits = 0;
    U16 mIdent = 0;
    U8 mCOM = 0;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif // VAN_ANALYZER_H
