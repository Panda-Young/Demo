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

void saveFloatPCMData(const juce::File &pcmFile, const float *data, size_t numSamples)
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

    std::ofstream wavFile(pcmFile.getFullPathName().toStdString() + ".wav", std::ios::binary | std::ios::out);
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

bool checkLicenseFile(const juce::File &licenseFile,
                      uint32_t nYear, uint32_t nMonth, uint32_t nDay,
                      uint32_t nHour, uint32_t nMinute, uint32_t nSecond)
{
    if (!licenseFile.existsAsFile()) {
        std::ofstream outFile(licenseFile.getFullPathName().toStdString());
        std::string currentDate = getCurrentDateString();
        std::string encryptedDate = encryptDate(currentDate);
        outFile << encryptedDate;
        outFile.close();

        // Modify the file creation time: This is necessary because even if the file is deleted and recreated,
        // the file system might retain the original creation time.
        HANDLE hFile = CreateFileW(licenseFile.getFullPathName().toWideCharPointer(),
                                   GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            SYSTEMTIME st;
            FILETIME ft;

            // Convert currentDate to std::tm structure
            std::istringstream iss(currentDate);
            std::tm current_tm;
            iss >> std::get_time(&current_tm, "%Y-%m-%d %H:%M:%S");

            // Convert std::tm to time_t
            std::time_t current_time_t = std::mktime(&current_tm);

            // Convert time_t to UTC time
            std::tm *utc_tm = std::gmtime(&current_time_t);

            // Set SYSTEMTIME structure
            st.wYear = utc_tm->tm_year + 1900;
            st.wMonth = utc_tm->tm_mon + 1;
            st.wDay = utc_tm->tm_mday;
            st.wHour = utc_tm->tm_hour;
            st.wMinute = utc_tm->tm_min;
            st.wSecond = utc_tm->tm_sec;
            st.wMilliseconds = 0;

            // Convert SYSTEMTIME to FILETIME
            SystemTimeToFileTime(&st, &ft);

            // Set the file creation time
            SetFileTime(hFile, &ft, NULL, NULL);
            CloseHandle(hFile);
        }

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

        // Calculate the expiration duration in seconds
        auto expiration_duration = std::chrono::seconds(nYear * 365 * 24 * 3600 +
                                                        nMonth * 30 * 24 * 3600 +
                                                        nDay * 24 * 3600 +
                                                        nHour * 3600 + nMinute * 60 + nSecond);

        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - creation_time);

        if (duration > expiration_duration) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "License Expired",
                                                   "Your license has expired.");
            return false;
        }

        // Calculate remaining time
        auto remaining_time = expiration_duration - duration;
        auto remaining_days = std::chrono::duration_cast<std::chrono::hours>(remaining_time).count() / 24;

        // If remaining time is less than 7 days, show a warning message
        if (remaining_days < 7) {
            auto expiration_time = creation_time + expiration_duration;
            std::time_t expiration_time_t = std::chrono::system_clock::to_time_t(expiration_time);
            std::tm expiration_tm = *std::localtime(&expiration_time_t);
            std::ostringstream oss;
            oss << std::put_time(&expiration_tm, "%Y-%m-%d %H:%M:%S");
            std::string expiration_str = oss.str();

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "License Warning",
                                                   "Your license will expire on " + expiration_str);
        }
    }
    return true;
}
