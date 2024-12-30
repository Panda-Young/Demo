/***************************************************************************
 * Description: Header of PluginProcessor
 * version: 0.1.0
 * Author: Panda-Young
 * @Date: 2024-09-02 13:14:56
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "myUtils.h"
#include "myLogger.h"
#include <JucePluginDefines.h>                   // Include this for JucePlugin_Name
#include <chrono>                                // Include this for std::chrono
#include <ctime>                                 // Include this for std::time_t, std::tm, std::localtime, std::mktime
#include <fstream>                               // Include this for std::ofstream, std::ifstream
#include <iomanip>                               // Include this for std::put_time
#include <juce_cryptography/juce_cryptography.h> // Include this for MD5
#include <regex>                                 // Include this for std::regex, std::smatch
#include <sstream>                               // Include this for std::ostringstream, std::istringstream

int getPluginType()
{
    juce::File dllPath = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    LOG_MSG_CF(LOG_INFO, "dllPath= \"%s\"", dllPath.getFullPathName().toRawUTF8());
    std::string path(dllPath.getFullPathName().toStdString());
    auto pos = path.rfind('.');
    if (pos != std::string::npos) {
        auto extension = path.substr(pos);
        if (extension == ".dll") {
            return 2;
        } else if (extension == ".vst3") {
            return 3;
        }
    }
    return -1;
}

std::string getHostAppName()
{
    juce::File hostAppFile = juce::File::getSpecialLocation(juce::File::hostApplicationPath);
    std::string tempHostAppName = hostAppFile.getFileNameWithoutExtension().toStdString();
    LOG_MSG(LOG_INFO, "hostAppPath = \"" + hostAppFile.getFullPathName().toStdString() + "\"");
    return tempHostAppName;
}

int getAuditionVersion()
{
    juce::File hostAppFile = juce::File::getSpecialLocation(juce::File::hostApplicationPath);
    std::string hostAppPath = hostAppFile.getFullPathName().toStdString();

    std::regex versionRegex(R"(Adobe Audition (\d+))");
    std::smatch matches;
    if (std::regex_search(hostAppPath, matches, versionRegex) && matches.size() > 1) {
        return std::stoi(matches[1]);
    }
    if (hostAppPath.find("Audition") != std::string::npos) {
        return 0;
    }
    return -1;
}

void dumpFloatPCMData(const juce::File &pcmFile, const float *data, size_t numSamples)
{
    std::ofstream outFile(pcmFile.getFullPathName().toStdString(), std::ios::binary | std::ios::app);
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to open file for writing: " + pcmFile.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
        return;
    }
    outFile.write(reinterpret_cast<const char *>(data), numSamples * sizeof(float));
    outFile.close();
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to write data to file: " + pcmFile.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
    }
}

void dumpFloatPCMData(const juce::File &pcmFile, const float *dataLeft, const float *dataRight, size_t numSamples)
{
    std::ofstream outFile(pcmFile.getFullPathName().toStdString(), std::ios::binary | std::ios::app);
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to open file for writing: " + pcmFile.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
        return;
    }
    for (size_t i = 0; i < numSamples; ++i) {
        outFile.write(reinterpret_cast<const char *>(&dataLeft[i]), sizeof(float));
        outFile.write(reinterpret_cast<const char *>(&dataRight[i]), sizeof(float));
    }
    outFile.close();
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to write data to file: " + pcmFile.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
    }
}

void dumpFloatBufferData(const juce::File &pcmFile, juce::AudioBuffer<float> &buffer)
{
    std::ofstream outFile(pcmFile.getFullPathName().toStdString(), std::ios::binary | std::ios::app);
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to open file for writing: " + pcmFile.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
        return;
    }

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            outFile.write(reinterpret_cast<const char *>(&buffer.getReadPointer(channel)[i]), sizeof(float));
        }
    }
    outFile.close();
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to write data to file: " + pcmFile.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
    }
}

void convertPCMtoWAV(const juce::File &pcmFile, uint16_t Num_Channel, uint32_t SampleRate,
                     uint16_t bits_per_sam, uint16_t audioFormat)
{
    if (!pcmFile.existsAsFile()) {
        LOG_MSG(LOG_ERROR, "PCM file does not exist: " + pcmFile.getFullPathName().toStdString());
        return;
    }
    if (pcmFile.getSize() == 0) {
        if (!pcmFile.deleteFile()) {
            LOG_MSG(LOG_ERROR, "Failed to delete empty PCM file: " + pcmFile.getFullPathName().toStdString());
        }
        return;
    }
    std::ifstream pcmFileStream(pcmFile.getFullPathName().toStdString(), std::ios::binary | std::ios::in);
    if (!pcmFileStream) {
        LOG_MSG(LOG_ERROR, "Failed to open file for reading: " + pcmFile.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
        return;
    }

    std::string newFilePath = pcmFile.getFullPathName().toStdString();
    newFilePath.replace(newFilePath.find(".pcm"), 4, ".wav");
    std::ofstream wavFile(newFilePath, std::ios::binary | std::ios::out);
    if (!wavFile) {
        LOG_MSG(LOG_ERROR, "Failed to open file for writing: " + pcmFile.getFullPathName().toStdString() +
                               ".wav. Reason: " + std::string(strerror(errno)));
        return;
    }

    // Calculate sizes
    pcmFileStream.seekg(0, std::ios::end);
    std::streamsize pcmSize = pcmFileStream.tellg();
    pcmFileStream.seekg(0, std::ios::beg);

    uint32_t byteRate = SampleRate * Num_Channel * bits_per_sam / 8;
    uint16_t blockAlign = Num_Channel * bits_per_sam / 8;
    uint32_t subchunk2Size = static_cast<uint32_t>(pcmSize);
    uint32_t chunkSize = 36 + subchunk2Size;

    // Write WAV header
    wavFile.write("RIFF", 4);
    wavFile.write(reinterpret_cast<const char *>(&chunkSize), 4);
    wavFile.write("WAVE", 4);
    wavFile.write("fmt ", 4);

    uint32_t subchunk1Size = 16;
    wavFile.write(reinterpret_cast<const char *>(&subchunk1Size), 4);
    wavFile.write(reinterpret_cast<const char *>(&audioFormat), 2);
    wavFile.write(reinterpret_cast<const char *>(&Num_Channel), 2);
    wavFile.write(reinterpret_cast<const char *>(&SampleRate), 4);
    wavFile.write(reinterpret_cast<const char *>(&byteRate), 4);
    wavFile.write(reinterpret_cast<const char *>(&blockAlign), 2);
    wavFile.write(reinterpret_cast<const char *>(&bits_per_sam), 2);

    wavFile.write("data", 4);
    wavFile.write(reinterpret_cast<const char *>(&subchunk2Size), 4);

    // Write PCM data to WAV file
    std::vector<char> buffer(static_cast<size_t>(pcmSize));
    if (pcmFileStream.read(buffer.data(), pcmSize)) {
        wavFile.write(buffer.data(), pcmSize);
    }

    // Close files
    pcmFileStream.close();
    wavFile.close();

    // Delete PCM file
    if (!pcmFile.deleteFile()) {
        LOG_MSG(LOG_ERROR, "Failed to delete PCM file: " + pcmFile.getFullPathName().toStdString());
    }
}

juce::String getSerial()
{
    std::string cpuID = "";
    if (!get_cpu_id(cpuID)) {
        LOG_MSG(LOG_WARN, "Failed to get CPU ID.");
    }
    if (cpuID.length() < 16) {
        cpuID = "0000000000000000";
    }

    std::string diskID = "";
    if (!get_disk_id(diskID)) {
        LOG_MSG(LOG_WARN, "Failed to get disk ID.");
    }
    if (diskID.length() < 8) {
        diskID = "00000000";
    }

    juce::String cpuId(cpuID.c_str());
    juce::String diskId(diskID.c_str());
    juce::String serial = cpuId.substring(0, 2) + diskId.substring(0, 1) + "-" +
                          cpuId.substring(2, 4) + diskId.substring(1, 2) + "-" +
                          cpuId.substring(4, 6) + diskId.substring(2, 3) + "-" +
                          cpuId.substring(6, 8) + diskId.substring(3, 4) + "-" +
                          cpuId.substring(8, 10) + diskId.substring(4, 5) + "-" +
                          cpuId.substring(10, 12) + diskId.substring(5, 6) + "-" +
                          cpuId.substring(12, 14) + diskId.substring(6, 7) + "-" +
                          cpuId.substring(14, 16) + diskId.substring(7, 8);
    return serial;
}

juce::String getRegSequence(const juce::String strSerial, RegType_t type)
{
    juce::String byte_array = strSerial;
    juce::String new_array;
    switch (type) {
    case UserReg:
        new_array = byte_array.replaceSection(5, 0, "U");
        break;
    case VIPReg:
        new_array = byte_array.replaceSection(5, 0, "P");
        break;
    default:
        break;
    }
    juce::MD5 md5(new_array.toUTF8());
    return md5.toHexString().toUpperCase();
}

juce::String hashStringFormat(const juce::String hashTemp, RegType_t type)
{
    juce::String rettemp = "";
    if (type == UserReg) {
        std::time_t t = std::time(nullptr);
        std::tm *now = std::localtime(&t);
        now->tm_mday += 7;
        std::mktime(now); // Normalize the date
        char dateStr[9] = {0};
        std::strftime(dateStr, sizeof(dateStr), "%m%d%Y", now);
        for (int i = 0; i < 8; i++) {
            dateStr[i] += 17; // interger to Upper case character
        }
        juce::String dateString(dateStr);
        rettemp += hashTemp.substring(0, 4) + dateString.substring(0, 1) + "-" +
                   hashTemp.substring(4, 8) + dateString.substring(0, 1) + "-" +
                   hashTemp.substring(8, 12) + dateString.substring(1, 2) + "-" +
                   hashTemp.substring(12, 16) + dateString.substring(3, 4) + "-" +
                   hashTemp.substring(16, 20) + dateString.substring(4, 5) + "-" +
                   hashTemp.substring(20, 24) + dateString.substring(5, 6) + "-" +
                   hashTemp.substring(24, 28) + dateString.substring(6, 7) + "-" +
                   hashTemp.substring(28, 32) + dateString.substring(7, 8);
    } else if (type == VIPReg) {
        for (int i = 0; i < 7; i++) {
            rettemp += hashTemp.substring(4 * i, (4 * i + 4)) + "-";
        }
        rettemp += hashTemp.substring(28, 32);
    } else {
        LOG_MSG(LOG_ERROR, "Unknown regType");
    }
    return rettemp;
}

juce::String reverseHashStringFormat(const juce::String &formattedHash, RegType_t type)
{
    juce::String originalHash = "";
    if (type == UserReg) {
        for (int i = 0; i < 8; ++i) {
            originalHash += formattedHash.substring(6 * i, 6 * i + 4);
        }
    } else if (type == VIPReg) {
        for (int i = 0; i < 7; i++) {
            originalHash += formattedHash.substring(5 * i, (5 * i + 4));
        }
        originalHash += formattedHash.substring(35, 39);
    } else {
        LOG_MSG(LOG_ERROR, "Unknown regType");
    }
    return originalHash;
}

bool isLicenseTimeValid(const juce::String &license)
{
    juce::String dateString;
    dateString += license.substring(4, 5);
    dateString += license.substring(10, 11);
    dateString += license.substring(16, 17);
    dateString += license.substring(22, 23);
    dateString += license.substring(28, 29);
    dateString += license.substring(34, 35);
    dateString += license.substring(40, 41);
    dateString += license.substring(46, 47);

    char dateStr[9] = {0};
    for (int i = 0; i < 8; i++) {
        dateStr[i] = dateString[i] - 17; // Upper case character to integer
    }

    std::tm licenseDate = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&licenseDate, "%m%d%Y");

    std::ostringstream oss;
    oss << std::put_time(&licenseDate, "%m/%d/%Y");

    juce::Time currentTime = juce::Time::getCurrentTime();
    juce::Time licenseTime(licenseDate.tm_year + 1900, licenseDate.tm_mon, licenseDate.tm_mday, 0, 0);
    return licenseTime >= currentTime;
}

RegType_t checkRegType()
{
    juce::File licenseDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Panda");
    char licenseFileName[64] = JucePlugin_Name;
    strcat(licenseFileName, "_VST_Plugin.lic");
    licenseFileName[strlen(licenseFileName)] = '\0';
    juce::File file = licenseDir.getChildFile(licenseFileName);
    juce::File currentExecutable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    juce::File tempLicense(currentExecutable.getParentDirectory().getChildFile(licenseFileName));

    if (file.exists()) {
        juce::FileInputStream input(file);
        if (input.openedOk()) {
            juce::String strLicense = input.readNextLine().trim();
            juce::String StrSerial = getSerial();
            if (strLicense.length() == 39) {
                if (strLicense == hashStringFormat(getRegSequence(StrSerial, VIPReg), VIPReg)) {
                    LOG_MSG(LOG_INFO, "VIP license is valid.");
                    return VIPReg;
                } else {
                    LOG_MSG(LOG_ERROR, "VIP License is invalid.");
                    return NoReg;
                }
            } else if (strLicense.length() == 47) {
                if (reverseHashStringFormat(strLicense, UserReg) == getRegSequence(StrSerial, UserReg)) {
                    if (isLicenseTimeValid(strLicense)) {
                        LOG_MSG(LOG_INFO, "User license is valid.");
                        return UserReg;
                    } else {
                        LOG_MSG(LOG_INFO, "User license time expired.");
                        return NoReg;
                    }
                } else {
                    LOG_MSG(LOG_ERROR, "User license is invalid.");
                    return NoReg;
                }
            } else {
                LOG_MSG(LOG_ERROR, "license is invalid.");
                return NoReg;
            }
        } else {
            LOG_MSG(LOG_ERROR, "Failed to open license file.");
            return NoReg;
        }
    } else {
        if (tempLicense.exists()) {
            LOG_MSG(LOG_INFO, "Temp license file exists.");
            juce::FileInputStream input(tempLicense);
            if (input.openedOk()) {
                juce::String strLicense = input.readNextLine().trim();
                if (isLicenseTimeValid(strLicense)) {
                    LOG_MSG(LOG_INFO, "Temp license is valid.");
                    return UserReg;
                } else {
                    LOG_MSG(LOG_INFO, "Temp license time expired.");
                    return NoReg;
                }
            } else {
                LOG_MSG(LOG_ERROR, "Failed to open temp license file.");
                return NoReg;
            }
        }
        LOG_MSG(LOG_INFO, "License file: " + file.getFullPathName().toStdString() + " does not exist.");
        return NoReg;
    }
}

RegType_t regSoftware(juce::String strLicense)
{
    juce::String StrSerial = getSerial();
    juce::File licenseDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Panda");
    if (!licenseDir.isDirectory()) {
        licenseDir.createDirectory();
    }
    char licenseFileName[64] = JucePlugin_Name;
    strcat(licenseFileName, "_VST_Plugin.lic");
    juce::String filePath = licenseDir.getFullPathName() + juce::String("/") + juce::String(licenseFileName);
    strLicense = strLicense.trim();
    if (strLicense.length() == 39) {
        if (strLicense == hashStringFormat(getRegSequence(StrSerial, VIPReg), VIPReg)) {
            juce::File file(filePath);
            juce::FileOutputStream out(file);
            out.setPosition(0);
            out.write(strLicense.getCharPointer(), strLicense.length());
            out.flush();
            if (file.exists()) {
                LOG_MSG(LOG_INFO, "Write VIP license to file: " + file.getFullPathName().toStdString());
                return VIPReg;
            } else {
                LOG_MSG(LOG_ERROR, "Failed to write VIP license to file.");
                return NoReg;
            }
        } else {
            LOG_MSG(LOG_ERROR, "VIP License is invalid.");
            return NoReg;
        }
    } else if (strLicense.length() == 47) {
        if (reverseHashStringFormat(strLicense, UserReg) == getRegSequence(StrSerial, UserReg)) {
            if (isLicenseTimeValid(strLicense)) {
                juce::File file(filePath);
                juce::FileOutputStream out(file);
                out.setPosition(0);
                out.write(strLicense.getCharPointer(), strLicense.length());
                out.flush();
                if (file.exists()) {
                    LOG_MSG(LOG_INFO, "Write User license to file: " + file.getFullPathName().toStdString());
                    return UserReg;
                } else {
                    LOG_MSG(LOG_ERROR, "Failed to write User license to file.");
                    return NoReg;
                }
            } else {
                LOG_MSG(LOG_ERROR, "User license time expired.");
                return NoReg;
            }
        } else {
            LOG_MSG(LOG_ERROR, "User license is invalid.");
            return NoReg;
        }
    } else {
        LOG_MSG_CF(LOG_ERROR, "license is invalid length: %d", strLicense.length());
        return NoReg;
    }
}
