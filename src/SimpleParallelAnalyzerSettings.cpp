#include "SimpleParallelAnalyzerSettings.h"
#include <AnalyzerHelpers.h>
#include <stdio.h>


#pragma warning( disable : 4996 ) // warning C4996: 'sprintf': This function or variable may be unsafe

SimpleParallelAnalyzerSettings::SimpleParallelAnalyzerSettings() : mShowValues( true ), mShowDots( true )
{
    U32 count = 16;
    for( U32 i = 0; i < count; i++ )
    {
        mDataChannels.push_back( UNDEFINED_CHANNEL );
        AnalyzerSettingInterfaceChannel* data_channel_interface = new AnalyzerSettingInterfaceChannel();

        char text[ 64 ];
        sprintf( text, "D%d", i );

        data_channel_interface->SetTitleAndTooltip( text, text );
        data_channel_interface->SetChannel( mDataChannels[ i ] );
        data_channel_interface->SetSelectionOfNoneIsAllowed( true );

        mDataChannelsInterface.push_back( data_channel_interface );
    }


    mShowValuesInterface.reset( new AnalyzerSettingInterfaceBool() );
    mShowValuesInterface->SetTitleAndTooltip( "Show Values", "Enable data values display on channel D0." );
    mShowValuesInterface->SetValue( mShowValues );

    mShowDotsInterface.reset( new AnalyzerSettingInterfaceBool() );
    mShowDotsInterface->SetTitleAndTooltip( "Show Dots", "Enable dots that marks data transition on all channels." );
    mShowDotsInterface->SetValue( mShowDots );


    for( U32 i = 0; i < count; i++ )
    {
        AddInterface( mDataChannelsInterface[ i ] );
    }

    AddInterface( mShowValuesInterface.get() );
    AddInterface( mShowDotsInterface.get() );

    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    for( U32 i = 0; i < count; i++ )
    {
        char text[ 64 ];
        sprintf( text, "D%d", i );
        AddChannel( mDataChannels[ i ], text, false );
    }
}

SimpleParallelAnalyzerSettings::~SimpleParallelAnalyzerSettings()
{
    U32 count = mDataChannelsInterface.size();
    for( U32 i = 0; i < count; i++ )
        delete mDataChannelsInterface[ i ];
}

bool SimpleParallelAnalyzerSettings::SetSettingsFromInterfaces()
{
    U32 count = mDataChannels.size();
    U32 num_used_channels = 0;
    for( U32 i = 0; i < count; i++ )
    {
        if( mDataChannelsInterface[ i ]->GetChannel() != UNDEFINED_CHANNEL )
            num_used_channels++;
    }

    if( num_used_channels == 0 )
    {
        SetErrorText( "Please select at least one channel to use in the parallel bus" );
        return false;
    }

    for( U32 i = 0; i < count; i++ )
    {
        mDataChannels[ i ] = mDataChannelsInterface[ i ]->GetChannel();
    }

    mShowValues = mShowValuesInterface->GetValue();
    mShowDots = mShowDotsInterface->GetValue();

    ClearChannels();
    for( U32 i = 0; i < count; i++ )
    {
        char text[ 64 ];
        sprintf( text, "D%d", i );
        AddChannel( mDataChannels[ i ], text, mDataChannels[ i ] != UNDEFINED_CHANNEL );
    }

    return true;
}

void SimpleParallelAnalyzerSettings::UpdateInterfacesFromSettings()
{
    U32 count = mDataChannels.size();
    for( U32 i = 0; i < count; i++ )
    {
        mDataChannelsInterface[ i ]->SetChannel( mDataChannels[ i ] );
    }

    mShowValuesInterface->SetValue( mShowValues );
    mShowDotsInterface->SetValue( mShowDots );
}

void SimpleParallelAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    U32 count = mDataChannels.size();

    for( U32 i = 0; i < count; i++ )
    {
        text_archive >> mDataChannels[ i ];
    }

    text_archive >> mShowValues;
    text_archive >> mShowDots;

    ClearChannels();
    for( U32 i = 0; i < count; i++ )
    {
        char text[ 64 ];
        sprintf( text, "D%d", i );
        AddChannel( mDataChannels[ i ], text, mDataChannels[ i ] != UNDEFINED_CHANNEL );
    }

    UpdateInterfacesFromSettings();
}

const char* SimpleParallelAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    U32 count = mDataChannels.size();

    for( U32 i = 0; i < count; i++ )
    {
        text_archive << mDataChannels[ i ];
    }

    text_archive << mShowValues;
    text_archive << mShowDots;

    return SetReturnString( text_archive.GetString() );
}
