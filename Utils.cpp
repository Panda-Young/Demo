/***************************************************************************
 * Description: Header of PluginProcessor
 * version: 0.1.0
 * Author: Panda-Young
 * @Date: 2024-09-02 13:14:56
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "Utils.h"
#include "Logger.h"
#include <JucePluginDefines.h>                   // Include this header for JucePlugin_Name
#include <chrono>                                // Include this header for std::chrono
#include <ctime>                                 // Include this header for std::time_t, std::tm, std::localtime, std::mktime
#include <fstream>                               // Include this header for std::ofstream, std::ifstream
#include <intrin.h>                              // Include this header for __cpuid
#include <iomanip>                               // Include this header for std::put_time
#include <juce_cryptography/juce_cryptography.h> // Include this header for MD5
#include <regex>                                 // Include this header for std::regex, std::smatch
#include <sstream>                               // Include this header for std::ostringstream, std::istringstream
#include <windows.h>

int getPluginType()
{
    juce::File dllPath = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    LOG_MSG_CF(LOG_INFO, "dllPath= \"%s\"", dllPath.getFullPathName().toRawUTF8());
    auto pos = dllPath.getFullPathName().toStdString().rfind('.');
    if (pos != std::string::npos) {
        auto extension = dllPath.getFullPathName().toStdString().substr(pos);
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

void dumpFloatBufferData(const juce::File &pcmFile, juce::AudioBuffer<float>& buffer)
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

BOOL GetCpuByCmd(char *lpszCpu, int len = 128)
{
    const long MAX_COMMAND_SIZE = 10000;           // Command line output buffer size
    char szFetCmd[] = "wmic cpu get processorid";  // Command line to get CPU serial number
    const std::string strEnSearch = "ProcessorId"; // Leading information of the CPU serial number

    BOOL bret = FALSE;
    HANDLE hReadPipe = NULL;  // Read pipe
    HANDLE hWritePipe = NULL; // Write pipe
    PROCESS_INFORMATION pi;   // Process information
    STARTUPINFO si;           // Control command line window information
    SECURITY_ATTRIBUTES sa;   // Security attributes

    char szBuffer[MAX_COMMAND_SIZE + 1] = {0}; // Output buffer for command line results
    std::string strBuffer;
    unsigned long count = 0;
    long ipos = 0;

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    memset(&sa, 0, sizeof(sa));

    pi.hProcess = NULL;
    pi.hThread = NULL;
    si.cb = sizeof(STARTUPINFO);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    // 1.0 Create pipe
    bret = CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    if (!bret) {
        goto END;
    }

    // 2.0 Set the command line window information to the specified read and write pipes
    GetStartupInfo(&si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.wShowWindow = SW_HIDE; // Hide the command line window
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

    // 3.0 Create a process to get the command line
    bret = CreateProcess(NULL, szFetCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (!bret) {
        goto END;
    }

    // 4.0 Read the returned data
    WaitForSingleObject(pi.hProcess, 500 /*INFINITE*/);
    bret = ReadFile(hReadPipe, szBuffer, MAX_COMMAND_SIZE, &count, 0);
    if (!bret) {
        goto END;
    }

    // 5.0 Find the CPU serial number
    bret = FALSE;
    strBuffer = szBuffer;
    ipos = strBuffer.find(strEnSearch);

    if (ipos < 0) // Not found
    {
        goto END;
    } else {
        strBuffer = strBuffer.substr(ipos + strEnSearch.length());
    }

    memset(szBuffer, 0x00, sizeof(szBuffer));
    strcpy_s(szBuffer, strBuffer.c_str());

    // Remove spaces, \r, \n
    {
        int j = 0;
        for (int i = 0; i < strlen(szBuffer); i++) {
            if (szBuffer[i] != ' ' && szBuffer[i] != '\n' && szBuffer[i] != '\r') {
                lpszCpu[j] = szBuffer[i];
                j++;
            }
        }
        lpszCpu[j] = '\0'; // Ensure the string is null-terminated
    }

    bret = TRUE;

END:
    // Close all handles
    CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return bret;
}

juce::String getHardDriveSerialNumber()
{
    DWORD serialNumber = 0;
    if (GetVolumeInformationW(L"C:\\", NULL, 0, &serialNumber, NULL, NULL, NULL, 0)) {
        return juce::String::toHexString((int)serialNumber).toUpperCase();
    }
    return "Unknown";
}

juce::String getSerial()
{
    char cpuid[64] = {0};
    if (!GetCpuByCmd(cpuid, 64)) {
        LOG_MSG(LOG_ERROR, "Failed to get CPU ID.");
    }
    juce::String cpuId(cpuid);
    juce::String diskId = getHardDriveSerialNumber();
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

bool isLicenseValid(const juce::String &license)
{
    juce::String dateString;
    dateString += license.substring(4, 6);   // MM
    dateString += license.substring(12, 14); // DD
    dateString += license.substring(20, 24); // YYYY

    char dateStr[9] = {0};
    for (int i = 0; i < 8; i++) {
        dateStr[i] = dateString[i] - 17; // Upper case character to integer
    }

    std::tm licenseDate = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&licenseDate, "%m%d%Y");

    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    std::time_t licenseTime = std::mktime(&licenseDate);
    std::time_t currentTime = std::mktime(now);

    return difftime(licenseTime, currentTime) >= 0;
}

RegType_t checkRegType()
{
    juce::File licenseDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Panda");
    char licenseFileName[64] = JucePlugin_Name;
    strcat(licenseFileName, "_VST_Plugin.lic");
    licenseFileName[strlen(licenseFileName)] = '\0';
    juce::File file(licenseDir.getFullPathName() + "\\" + licenseFileName);
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
                    if (isLicenseValid(strLicense)) {
                        LOG_MSG(LOG_INFO, "User license is valid.");
                        return UserReg;
                    } else {
                        LOG_MSG(LOG_INFO, "User license is invalid.");
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
    } else if (tempLicense.exists()) {
        LOG_MSG(LOG_INFO, "Temp license file exists.");
        juce::FileInputStream input(tempLicense);
        if (input.openedOk()) {
            juce::String strLicense = input.readNextLine().trim();
            if (isLicenseValid(strLicense)) {
                LOG_MSG(LOG_INFO, "Temp license is valid.");
                return UserReg;
            } else {
                LOG_MSG(LOG_INFO, "Temp license is invalid.");
                return NoReg;
            }
        } else {
            LOG_MSG(LOG_ERROR, "Failed to open temp license file.");
            return NoReg;
        }
    } else {
        LOG_MSG(LOG_INFO, "License file does not exist.");
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
    juce::String filePath = licenseDir.getFullPathName() + "/" + licenseFileName;
    if (strLicense.length() == 39) {
        if (strLicense == hashStringFormat(getRegSequence(StrSerial, VIPReg), VIPReg)) {
            juce::File file(filePath);
            juce::FileOutputStream out(file);
            out.setPosition(0);
            out.write(strLicense.getCharPointer(), strLicense.length());
            out.flush();
            return VIPReg;
        } else {
            return NoReg;
        }
    } else if (strLicense.length() == 47) {
        if (reverseHashStringFormat(strLicense, UserReg) == getRegSequence(StrSerial, UserReg)) {
            if (isLicenseValid(strLicense)) {
                juce::File file(filePath);
                juce::FileOutputStream out(file);
                out.setPosition(0);
                out.write(strLicense.getCharPointer(), strLicense.length());
                out.flush();
                return UserReg;
            } else {
                return NoReg;
            }
        } else {
            return NoReg;
        }
    } else {
        return NoReg;
    }
}
