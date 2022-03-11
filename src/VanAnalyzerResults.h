#ifndef VAN_ANALYZER_RESULTS
#define VAN_ANALYZER_RESULTS

#include <AnalyzerResults.h>

enum VanFrameType { StartOfFrame, IdentifierField, CommandField, DataField, FCSField, EndOfData, AckField, EndOfFrame, VanError };

class VanAnalyzer;
class VanAnalyzerSettings;

class VanAnalyzerResults : public AnalyzerResults
{
public:
    VanAnalyzerResults( VanAnalyzer* analyzer, VanAnalyzerSettings* settings );
    virtual ~VanAnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
    VanAnalyzerSettings* mSettings;
    VanAnalyzer* mAnalyzer;
};

#endif //VAN_ANALYZER_RESULTS
