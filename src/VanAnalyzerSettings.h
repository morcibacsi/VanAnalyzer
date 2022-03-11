#ifndef VAN_ANALYZER_SETTINGS
#define VAN_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class VanAnalyzerSettings : public AnalyzerSettings
{
public:
    VanAnalyzerSettings();
    virtual ~VanAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();

    Channel mInputChannel;
    U32 mBitRate;
    bool mInverted;

    BitState Recessive();
    BitState Dominant();


protected:
    std::auto_ptr< AnalyzerSettingInterfaceChannel > mInputChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceInteger > mBitRateInterface;
    std::auto_ptr< AnalyzerSettingInterfaceBool > mVanChannelInvertedInterface;
};

#endif //VAN_ANALYZER_SETTINGS
