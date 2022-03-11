#include "VanAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


VanAnalyzerSettings::VanAnalyzerSettings()
:   mInputChannel( UNDEFINED_CHANNEL ),
    mBitRate( 125000 )
{
    mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelInterface->SetTitleAndTooltip( "VAN bus", "Standard VAN" );
    mInputChannelInterface->SetChannel( mInputChannel );

    mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
    mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
    mBitRateInterface->SetMax( 6000000 );
    mBitRateInterface->SetMin( 1 );
    mBitRateInterface->SetInteger( mBitRate );

    mVanChannelInvertedInterface.reset(new AnalyzerSettingInterfaceBool());
    mVanChannelInvertedInterface->SetTitleAndTooltip("Inverted (VAN High - DATA)", "Use this option when recording VAN High (DATA) directly");
    mVanChannelInvertedInterface->SetValue(mInverted);

    AddInterface(mInputChannelInterface.get());
    AddInterface(mBitRateInterface.get());
    AddInterface(mVanChannelInvertedInterface.get());

    AddExportOption( 0, "Export as text file" );
    AddExportExtension( 0, "text", "txt" );

    ClearChannels();
    AddChannel( mInputChannel, "VAN", false );
}

VanAnalyzerSettings::~VanAnalyzerSettings()
{
}

bool VanAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannel = mInputChannelInterface->GetChannel();
    
    if (mInputChannel == UNDEFINED_CHANNEL)
    {
        SetErrorText("Please select a channel for the VAN interface");
        return false;
    }

    mBitRate = mBitRateInterface->GetInteger();
    mInverted = mVanChannelInvertedInterface->GetValue();

    ClearChannels();
    AddChannel( mInputChannel, "VAN", true );

    return true;
}

void VanAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface->SetChannel( mInputChannel );
    mBitRateInterface->SetInteger( mBitRate );
    mVanChannelInvertedInterface->SetValue(mInverted);
}

void VanAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    const char* name_string;    //the first thing in the archive is the name of the protocol analyzer that the data belongs to.
    text_archive >> &name_string;
    if (strcmp(name_string, "SaleaeVANAnalyzer") != 0)
        AnalyzerHelpers::Assert("SaleaeVanAnalyzer: Provided with a settings string that doesn't belong to us;");


    text_archive >> mInputChannel;
    text_archive >> mBitRate;
    text_archive >> mInverted; //SimpleArchive catches exception and returns false if it fails.

    ClearChannels();
    AddChannel( mInputChannel, "VAN", true );

    UpdateInterfacesFromSettings();
}

const char* VanAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << "SaleaeVANAnalyzer";
    text_archive << mInputChannel;
    text_archive << mBitRate;
    text_archive << mInverted;

    return SetReturnString( text_archive.GetString() );
}

BitState VanAnalyzerSettings::Recessive()
{
    if (mInverted)
        return BIT_LOW;
    return BIT_HIGH;
}
BitState VanAnalyzerSettings::Dominant()
{
    if (mInverted)
        return BIT_HIGH;
    return BIT_LOW;
}
