#ifndef VAN_ANALYZER_H
#define VAN_ANALYZER_H

#include <Analyzer.h>
#include "VanAnalyzerResults.h"
#include "VanSimulationDataGenerator.h"

class VanAnalyzerSettings;
class ANALYZER_EXPORT VanAnalyzer : public Analyzer2
{
public:
    VanAnalyzer();
    virtual ~VanAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

protected: //vars
    std::auto_ptr< VanAnalyzerSettings > mSettings;
    std::auto_ptr< VanAnalyzerResults > mResults;
    AnalyzerChannelData* mSerial;

    VanSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    void WaitFor8RecessiveBits();
    void AddFrame(U64 startingPoint, U64 endingpoint, U32 data1, U32 type, U32 data2);
    bool GetNibble(U8& nibbleRead, U64& sampleNumberToReturnAsFrameEnding);
    U8 GetByte(U64& sampleNumberToReturnAsFrameEnding);
    U8 Get2Bits(U64& sampleNumberToReturnAsFrameEnding);

    //VAN analysis vars:
    U32 mSampleRateHz;
    U32 mSamplesPerBit;

    //VAN vars
    U32 mNumSamplesIn8Bits;
    U32 mStartOfFrame;
    U32 mIdentifier;
    U32 mCommand;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //VAN_ANALYZER_H
