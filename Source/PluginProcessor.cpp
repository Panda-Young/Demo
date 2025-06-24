#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <windows.h>
#include <cassert>

#define PIPE_NAME            L"\\\\.\\pipe\\VSTAudioPipe"
#define EVENT_TO_CHILD_NAME  L"Global\\VST_Event_To_Child"
#define EVENT_TO_PARENT_NAME L"Global\\VST_Event_To_Parent"

//==============================================================================
// IPCProcess implementation

IPCProcess::IPCProcess()
    : hPipe(INVALID_HANDLE_VALUE), hEventToChild(NULL), hEventToParent(NULL), childProcess(NULL)
{}

IPCProcess::~IPCProcess()
{
    stop();
}

bool IPCProcess::start()
{
    stop();

    // Create synchronization events
    hEventToChild = CreateEventW(NULL, FALSE, FALSE, EVENT_TO_CHILD_NAME); // auto-reset
    hEventToParent = CreateEventW(NULL, FALSE, FALSE, EVENT_TO_PARENT_NAME);
    if (!hEventToChild || !hEventToParent)
        return false;

    // Launch 64-bit child process (ensure path is correct or use relative path)
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"D:\\WorkSpace\\JUCE\\Demo\\VSTAudioChildProcess64.exe";
    BOOL ok = CreateProcessW(
        NULL, &cmd[0], NULL, NULL, FALSE,
        0, NULL, NULL, &si, &pi
    );
    if (!ok)
        return false;
    CloseHandle(pi.hThread);
    childProcess = pi.hProcess;

    // Wait for the child process to create the pipe
    int retry = 0;
    while (retry < 20)
    {
        hPipe = CreateFileW(
            PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, 0, NULL
        );
        if (hPipe != INVALID_HANDLE_VALUE)
            break;
        Sleep(100);
        ++retry;
    }

    started = (hPipe != INVALID_HANDLE_VALUE);
    return started;
}

void IPCProcess::stop()
{
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
    if (hEventToChild)
    {
        CloseHandle(hEventToChild);
        hEventToChild = NULL;
    }
    if (hEventToParent)
    {
        CloseHandle(hEventToParent);
        hEventToParent = NULL;
    }
    if (childProcess)
    {
        TerminateProcess(childProcess, 0);
        CloseHandle(childProcess);
        childProcess = NULL;
    }
    started = false;
}

bool IPCProcess::process(float* data, int numSamples, int numChannels)
{
    if (!started || hPipe == INVALID_HANDLE_VALUE)
        return false;

    DWORD nBytes;
    int N = numSamples, C = numChannels;
    if (!WriteFile(hPipe, &N, sizeof(int), &nBytes, NULL)) return false;
    if (!WriteFile(hPipe, &C, sizeof(int), &nBytes, NULL)) return false;
    int total = N * C;
    if (!WriteFile(hPipe, data, total * sizeof(float), &nBytes, NULL)) return false;

    SetEvent(hEventToChild);

    // Wait for child to finish processing
    WaitForSingleObject(hEventToParent, INFINITE);

    if (!ReadFile(hPipe, data, total * sizeof(float), &nBytes, NULL)) return false;
    return true;
}

//==============================================================================
// DemoAudioProcessor implementation

DemoAudioProcessor::DemoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    ipc = std::make_unique<IPCProcess>();
    ipc->start();
}

DemoAudioProcessor::~DemoAudioProcessor()
{
    if (ipc) ipc->stop();
}

void DemoAudioProcessor::prepareToPlay(double, int) {}
void DemoAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DemoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}
#endif

void DemoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    int N = buffer.getNumSamples();
    int C = buffer.getNumChannels();
    std::vector<float> data(N * C);
    // Interleave channel data
    for (int ch = 0; ch < C; ++ch)
        for (int i = 0; i < N; ++i)
            data[ch * N + i] = buffer.getSample(ch, i);

    if (ipc)
        ipc->process(data.data(), N, C);

    // Write back processed data
    for (int ch = 0; ch < C; ++ch)
        for (int i = 0; i < N; ++i)
            buffer.setSample(ch, i, data[ch * N + i]);
}

bool DemoAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* DemoAudioProcessor::createEditor()
{
    return new DemoAudioProcessorEditor(*this);
}

void DemoAudioProcessor::getStateInformation(juce::MemoryBlock&) {}
void DemoAudioProcessor::setStateInformation(const void*, int) {}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoAudioProcessor();
}
