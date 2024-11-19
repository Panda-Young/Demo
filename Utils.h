/***************************************************************************
 * Description: Header of PluginProcessor
 * version: 0.1.0
 * Author: Panda-Young
 * @Date: 2024-09-02 13:14:54
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <JuceHeader.h>
#include <string>

typedef enum RegType {
    NoReg = 0,
    UserReg = 1,
    VIPReg
} RegType_t;

int getPluginType();
std::string extractHostAppName();
int getAuditionVersion();
void dumpFloatPCMData(const juce::File &pcmFile, const float *data, size_t numSamples);
void dumpFloatPCMData(const juce::File &pcmFile, const float *dataLeft,
                      const float *dataRight, size_t numSamples);
void dumpFloatBufferData(const juce::File &pcmFile, juce::AudioBuffer<float>& buffer);
void convertPCMtoWAV(const juce::File &pcmFile, uint16_t Num_Channel, uint32_t SampleRate,
                     uint16_t bits_per_sam = 32, uint16_t audioFormat = 3);
bool checkLicenseFile(const juce::File &licenseFile, uint32_t nYear = 0, uint32_t nMonth = 0, uint32_t nDay = 7,
                      uint32_t nHour = 0, uint32_t nMinute = 0, uint32_t nSecond = 0);
juce::String getSerial();
RegType_t checkRegType();
RegType_t regSoftware(juce::String strLicense);

#endif // UTILS_H
