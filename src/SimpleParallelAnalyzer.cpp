#include "SimpleParallelAnalyzer.h"
#include "SimpleParallelAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <thread>

static const uint64_t DATA_IDLE = UINT64_MAX - 1uLL;
static const uint64_t DATA_ENDED = UINT64_MAX;

SimpleParallelAnalyzer::SimpleParallelAnalyzer()
    : Analyzer2(), mSettings( new SimpleParallelAnalyzerSettings() ), mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );
    UseFrameV2();
}

SimpleParallelAnalyzer::~SimpleParallelAnalyzer()
{
    KillThread();
}

void SimpleParallelAnalyzer::SetupResults()
{
    mResults.reset( new SimpleParallelAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    if( mSettings->mShowValues )
    {
        mResults->AddChannelBubblesWillAppearOn( mSettings->mDataChannels[ 0 ] );
    }
}

uint64_t SimpleParallelAnalyzer::GetNextTransition( uint64_t location )
{
    uint64_t result = DATA_ENDED;

    for( int i = 0; i < mData.size(); i++ )
    {
        if( mDataNextEdge[ i ] <= location || mDataNextEdge[ i ] == DATA_IDLE )
        {
            try
            {
                if( mData[ i ]->DoMoreTransitionsExistInCurrentData() )
                {
                    mDataNextEdge[ i ] = mData[ i ]->GetSampleOfNextEdge();
                }
                else if( mLiveCaptureEnded )
                {
                    mDataNextEdge[ i ] = DATA_ENDED;
                }
                else
                {
                    mDataNextEdge[ i ] = DATA_IDLE;
                }
            }
            catch( ... )
            {
                mLiveCaptureEnded = true;
                mDataNextEdge[ i ] = DATA_ENDED;
            }
        }

        if( result > mDataNextEdge[ i ] )
        {
            result = mDataNextEdge[ i ];
        }
    }

    return result;
}

void SimpleParallelAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();
    mLiveCaptureEnded = false;
    mData.clear();
    mDataMasks.clear();
    mDataChannels.clear();
    mDataNextEdge.clear();

    U32 count = mSettings->mDataChannels.size();
    U64 location = 0;
    U64 nextLocation = 0;
    for( U32 i = 0; i < count; i++ )
    {
        if( mSettings->mDataChannels[ i ] != UNDEFINED_CHANNEL )
        {
            auto chData = GetAnalyzerChannelData( mSettings->mDataChannels[ i ] );
            mData.push_back( chData );
            mDataMasks.push_back( 1 << i );
            mDataChannels.push_back( mSettings->mDataChannels[ i ] );
            mDataNextEdge.push_back( chData->DoMoreTransitionsExistInCurrentData() ? chData->GetSampleOfNextEdge() : DATA_IDLE );
            location = std::max( location, chData->GetSampleNumber() );
        }
    }

    bool dataEnded = false;

    while( !dataEnded )
    {
        U16 result = GetWordAtLocation( location );
        do
        {
            nextLocation = GetNextTransition( location );
            if( nextLocation == DATA_ENDED )
            {
                nextLocation = location + 1;
                dataEnded = true;
                break;
            }
            else if( nextLocation < DATA_IDLE )
            {
                break;
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            CheckIfThreadShouldExit();
        } while( true );

        Frame frame;
        frame.mData1 = result;
        frame.mType = 0;
        frame.mFlags = 0;
        frame.mStartingSampleInclusive = location;
        frame.mEndingSampleInclusive = std::max( location + 1, nextLocation - 1 );
        mResults->AddFrame( frame );

        FrameV2 frame_v2;
        frame_v2.AddInteger( "data", result );
        mResults->AddFrameV2( frame_v2, "data", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );
        mResults->CommitResults();

        if( dataEnded )
        {
            Frame stopFrame;
            stopFrame.mData1 = result;
            stopFrame.mType = 1;
            stopFrame.mFlags = 0;
            stopFrame.mStartingSampleInclusive = location + 2;
            stopFrame.mEndingSampleInclusive = location + 3;
            mResults->AddFrame( stopFrame );

            FrameV2 stopFrame_v2;
            mResults->AddFrameV2( stopFrame_v2, "stop", location + 2, location + 3 );
            mResults->CommitResults();
        }

        CheckIfThreadShouldExit();
        location = nextLocation;
        ReportProgress( nextLocation );
    }
}

bool SimpleParallelAnalyzer::NeedsRerun()
{
    return false;
}

U32 SimpleParallelAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                                    SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitilized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 SimpleParallelAnalyzer::GetMinimumSampleRateHz()
{
    return 1;
}

const char* SimpleParallelAnalyzer::GetAnalyzerName() const
{
    return "Clockless Parallel";
}


uint16_t SimpleParallelAnalyzer::GetWordAtLocation( uint64_t sample_number )
{
    uint16_t result = 0;

    int num_data_lines = mData.size();

    for( int i = 0; i < num_data_lines; i++ )
    {
        mData[ i ]->AdvanceToAbsPosition( sample_number );
        if( mData[ i ]->GetBitState() == BIT_HIGH )
        {
            result |= mDataMasks[ i ];
        }
        if( mSettings->mShowDots )
        {
            mResults->AddMarker( sample_number, AnalyzerResults::Dot, mDataChannels[ i ] );
        }
    }

    return result;
}

uint64_t SimpleParallelAnalyzer::AddFrame( uint16_t value, uint64_t starting_sample, uint64_t ending_sample )
{
    assert( starting_sample <= ending_sample );
    FrameV2 frame_v2;
    frame_v2.AddInteger( "data", value );

    Frame frame;
    frame.mData1 = value;
    frame.mFlags = 0;
    frame.mStartingSampleInclusive = starting_sample;
    frame.mEndingSampleInclusive = ending_sample;
    mResults->AddFrame( frame );
    mResults->AddFrameV2( frame_v2, "data", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );
    mResults->CommitResults();
    mLastFrameWidth = std::max<uint64_t>( ending_sample - starting_sample, 1 );
    return ending_sample;
}

const char* GetAnalyzerName()
{
    return "Clockless Parallel";
}

Analyzer* CreateAnalyzer()
{
    return new SimpleParallelAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}
