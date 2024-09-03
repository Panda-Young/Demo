/***************************************************************************
 * Description: Header of PluginProcessor
 * version: 0.1.0
 * Author: Panda-Young
 * @Date: 2024-09-02 13:14:56
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "Utils.h"
#include "Logger.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <windows.h>

int getPluginType(const std::string &dllPath)
{
    auto pos = dllPath.rfind('.');
    if (pos != std::string::npos) {
        auto extension = dllPath.substr(pos);
        if (extension == ".dll") {
            return 2;
        } else if (extension == ".vst3") {
            return 3;
        }
    }
    return -1;
}

std::string extractHostAppName()
{
    char hostAppPath[1024] = {0};
    GetModuleFileNameA(NULL, hostAppPath, sizeof(hostAppPath));
    LOG_MSG_CF(LOG_INFO, "hostAppPath= \"%s\"", hostAppPath);
    std::string path(hostAppPath);
    auto pos = path.find_last_of('\\');
    std::string tempHostAppName = (pos != std::string::npos) ? path.substr(pos + 1) : path;

    pos = tempHostAppName.find(".exe");
    if (pos != std::string::npos) {
        tempHostAppName = tempHostAppName.substr(0, pos);
    }

    return tempHostAppName;
}

int getAuditionVersion()
{
    char hostAppPath[1024] = {0};
    GetModuleFileNameA(NULL, hostAppPath, sizeof(hostAppPath));
    std::string path(hostAppPath);
    std::regex versionRegex(R"(Adobe Audition (\d+))");
    std::smatch matches;
    if (std::regex_search(path, matches, versionRegex) && matches.size() > 1) {
        return std::stoi(matches[1]);
    }
    if (path.find("Audition") != std::string::npos) {
        return 0;
    }
    return -1;
}

void savePCMDatatoDesktop(const std::string &filename, const float *data, size_t numSamples)
{
    juce::File desktopPath = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    juce::File PCMfullPath = desktopPath.getChildFile(filename);

    std::ofstream outFile(PCMfullPath.getFullPathName().toStdString(), std::ios::binary | std::ios::app);
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to open file for writing: " + PCMfullPath.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
        return;
    }
    outFile.write(reinterpret_cast<const char *>(data), numSamples * sizeof(float));
    outFile.close();
    if (!outFile) {
        LOG_MSG(LOG_ERROR, "Failed to write data to file: " + PCMfullPath.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
    }
}

void convertPCMtoWAV(const std::string &filename, uint16_t Num_Channel, uint32_t SampleRate, uint16_t bits_per_sam, uint16_t audioFormat)
{
    juce::File desktopPath = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    juce::File PCMfullPath = desktopPath.getChildFile(filename);

    std::ifstream pcmFile(PCMfullPath.getFullPathName().toStdString(), std::ios::binary | std::ios::in);
    if (!pcmFile) {
        LOG_MSG(LOG_ERROR, "Failed to open file for reading: " + PCMfullPath.getFullPathName().toStdString() +
                               ". Reason: " + std::string(strerror(errno)));
        return;
    }

    std::ofstream wavFile(PCMfullPath.getFullPathName().toStdString() + ".wav", std::ios::binary | std::ios::out);
    if (!wavFile) {
        LOG_MSG(LOG_ERROR, "Failed to open file for writing: " + PCMfullPath.getFullPathName().toStdString() +
                               ".wav. Reason: " + std::string(strerror(errno)));
        return;
    }

    // Calculate sizes
    pcmFile.seekg(0, std::ios::end);
    std::streamsize pcmSize = pcmFile.tellg();
    pcmFile.seekg(0, std::ios::beg);

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
    if (pcmFile.read(buffer.data(), pcmSize)) {
        wavFile.write(buffer.data(), pcmSize);
    }

    // Close files
    pcmFile.close();
    wavFile.close();

    // Delete PCM file
    if (!PCMfullPath.deleteFile()) {
        LOG_MSG(LOG_ERROR, "Failed to delete PCM file: " + PCMfullPath.getFullPathName().toStdString());
    }
}

std::string encryptDate(const std::string &date)
{
    std::string encrypted = date;
    const std::string key = "Demo";
    for (size_t i = 0; i < date.size(); ++i) {
        encrypted[i] ^= key[i % key.size()];
    }
    return encrypted;
}

std::string decryptDate(const std::string &encryptedDate)
{
    return encryptDate(encryptedDate);
}

std::string getCurrentDateString()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string getFileCreationDate(const juce::File &file)
{
    auto creationTime = file.getCreationTime();
    std::time_t creation_time_t = creationTime.toMilliseconds() / 1000;
    std::tm creation_tm = *std::localtime(&creation_time_t);
    std::ostringstream oss;
    oss << std::put_time(&creation_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool checkLicenseFile(const juce::File &licenseFile)
{
    if (!licenseFile.existsAsFile()) {
        std::ofstream outFile(licenseFile.getFullPathName().toStdString());
        std::string currentDate = getCurrentDateString();
        std::string encryptedDate = encryptDate(currentDate);
        outFile << encryptedDate;
        outFile.close();
        return true;
    } else {
        std::ifstream inFile(licenseFile.getFullPathName().toStdString());
        std::string encryptedDate;
        std::getline(inFile, encryptedDate);
        inFile.close();
        std::string decryptedDate = decryptDate(encryptedDate);
        std::string fileCreationDate = getFileCreationDate(licenseFile);

        if (decryptedDate != fileCreationDate) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "License Error",
                                                   "License file is corrupted or tampered.");
            return false;
        }

        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time);
        std::istringstream iss(decryptedDate);
        std::tm creation_tm;
        iss >> std::get_time(&creation_tm, "%Y-%m-%d %H:%M:%S");
        auto creation_time = std::chrono::system_clock::from_time_t(std::mktime(&creation_tm));
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - creation_time).count();

        if (duration > 7 * 24) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "License Expired",
                                                   "Your license has expired.");
            return false;
        }
    }
    return true;
}
