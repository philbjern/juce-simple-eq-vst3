/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    SLOPE_12,
    SLOPE_24,
    SLOPE_36,
    SLOPE_48
};

struct ChainSettings
{
    float peakFreq { 0 }, peakGainInDecibels { 0 }, peakQuality { 1.0f };
    float lowCutFreq { 0 }, highCutFreq { 0 };
    Slope lowCutSlope { Slope::SLOPE_12 }, highCutSlope { Slope::SLOPE_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};
    
private:
    
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    MonoChain leftChain, rightChain;
    
    enum ChainPositions
    {
        LOW_CUT,
        PEAK,
        HIGH_CUT
    };
    
    void updatePeakFilter(const ChainSettings& chainSettings);
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);
    
    template<int Index, typename ChainType, typename CoefficientType>
    void updateCoefficientsAndMakeBypassFalse(ChainType& chain,
                                              const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }
    
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& chain,
                         const CoefficientType& coefficients,
                         const Slope& slope)
    {
        chain.template setBypassed<0>(true);
        chain.template setBypassed<1>(true);
        chain.template setBypassed<2>(true);
        chain.template setBypassed<3>(true);
        
        switch (slope)
        {
            case Slope::SLOPE_48:
                updateCoefficientsAndMakeBypassFalse<3>(chain, coefficients);
                
            case Slope::SLOPE_36:
                updateCoefficientsAndMakeBypassFalse<2>(chain, coefficients);
                
            case Slope::SLOPE_24:
                updateCoefficientsAndMakeBypassFalse<1>(chain, coefficients);
                
            case Slope::SLOPE_12:
                updateCoefficientsAndMakeBypassFalse<0>(chain, coefficients);
                break;
        }
    }
    
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    
    void updateFilters();
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
