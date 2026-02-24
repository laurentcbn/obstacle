#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Single-page browser: only allows our resource-provider root
// ─────────────────────────────────────────────────────────────────────────────
struct SinglePageBrowser : juce::WebBrowserComponent
{
    using WebBrowserComponent::WebBrowserComponent;

    bool pageAboutToLoad (const juce::String& newURL) override
    {
        return newURL.startsWith (juce::WebBrowserComponent::getResourceProviderRoot())
            || newURL == "about:blank";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
class ObstacleEditor : public juce::AudioProcessorEditor,
                        public juce::Timer
{
public:
    explicit ObstacleEditor (ObstacleProcessor&);
    ~ObstacleEditor() override;

    void paint   (juce::Graphics&) override {}
    void resized () override;
    void timerCallback() override;

private:
    ObstacleProcessor& proc;

    // ── helpers ───────────────────────────────────────────────────────────────
    std::optional<juce::WebBrowserComponent::Resource>
        getResource (const juce::String& url);

    juce::var buildPatternVar()  const;
    juce::var buildStateVar()    const;
    void      setParam (const juce::String& name, float value);

    // ── state ─────────────────────────────────────────────────────────────────
    int  lastStep    = -1;
    bool uiPlaying   = false;

    // ── WebView (must come AFTER proc in declaration order) ───────────────────
    SinglePageBrowser webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObstacleEditor)
};
