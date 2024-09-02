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

int getPluginType(const std::string &dllPath);
std::string extractHostAppName();
int getAuditionVersion();
void savePCMDatatoDesktop(const std::string &filename, const float *data, size_t numSamples);
void convertPCMtoWAV(const std::string &filename, uint16_t Num_Channel, uint32_t SampleRate, uint16_t bits_per_sam, uint16_t audioFormat);

#endif // UTILS_H
