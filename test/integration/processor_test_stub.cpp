#include "PluginProcessor.h"

// Stub for the test build: replaces the production createEditor() (which
// would drag in DoomVizEditor → DoomViewport → OpenGL → DoomEngine → the
// whole renderer/scene chain). Tests instantiate the processor directly
// and never call createEditor(), but JUCE's AudioProcessor base class
// declares it pure-ish and we must provide a definition for the link.
juce::AudioProcessorEditor* DoomVizProcessor::createEditor()
{
    return nullptr;
}
