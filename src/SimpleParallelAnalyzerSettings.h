#ifndef SIMPLEPARALLEL_ANALYZER_SETTINGS
#define SIMPLEPARALLEL_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

// originally from AnalyzerEnums::EdgeDirection { PosEdge, NegEdge };
enum class ParallelAnalyzerClockEdge
{
    PosEdge = AnalyzerEnums::PosEdge,
    NegEdge = AnalyzerEnums::NegEdge,
    DualEdge
};

class SimpleParallelAnalyzerSettings : public AnalyzerSettings
{
  public:
    SimpleParallelAnalyzerSettings();
    virtual ~SimpleParallelAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();


    std::vector<Channel> mDataChannels;
    bool mShowValues;
    bool mShowDots;

  protected:
    std::vector<AnalyzerSettingInterfaceChannel*> mDataChannelsInterface;

    std::unique_ptr<AnalyzerSettingInterfaceBool> mShowValuesInterface;
    std::unique_ptr<AnalyzerSettingInterfaceBool> mShowDotsInterface;
};

#endif // SIMPLEPARALLEL_ANALYZER_SETTINGS
