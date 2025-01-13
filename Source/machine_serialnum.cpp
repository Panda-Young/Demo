#include "myUtils.h"

#if JUCE_WINDOWS
#include <intrin.h>  // Include this for cpuid
#include <windows.h> // Include this for get processorid

bool get_cpu_id(std::string &cpu_id)
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
        for (size_t i = 0; i < strlen(szBuffer); i++) {
            if (szBuffer[i] != ' ' && szBuffer[i] != '\n' && szBuffer[i] != '\r') {
                cpu_id += szBuffer[i];
                j++;
            }
        }
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

bool get_disk_id(std::string &disk_id)
{
    DWORD serialNumber = 0;
    if (GetVolumeInformationW(L"C:\\", NULL, 0, &serialNumber, NULL, NULL, NULL, 0)) {
        disk_id = juce::String::toHexString((int)serialNumber).toUpperCase().toStdString();
        return TRUE;
    }
    return FALSE;
}

#else

#include <arpa/inet.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/hdreg.h>
#include <scsi/sg.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

static bool get_cpu_id_by_asm(std::string &cpu_id)
{
    cpu_id.clear();

    unsigned int s1 = 0;
    unsigned int s2 = 0;
    asm volatile(
        "movl $0x01, %%eax; \n\t"
        "xorl %%edx, %%edx; \n\t"
        "cpuid; \n\t"
        "movl %%edx, %0; \n\t"
        "movl %%eax, %1; \n\t"
        : "=m"(s1), "=m"(s2));
    if (0 == s1 && 0 == s2) {
        return (false);
    }
    char cpu[128];
    memset(cpu, 0, sizeof(cpu));
    snprintf(cpu, sizeof(cpu), "%08X%08X", htonl(s2), htonl(s1));
    cpu_id.assign(cpu);
    return (true);
}

static void parse_cpu_id(const char *file_name, const char *match_words, std::string &cpu_id)
{
    std::ifstream ifs(file_name, std::ios::binary);
    if (!ifs.is_open()) {
        return;
    }

    char line[4096] = {0};
    while (!ifs.eof()) {
        ifs.getline(line, sizeof(line));
        if (!ifs.good()) {
            break;
        }

        const char *cpu = strstr(line, match_words);
        if (NULL == cpu) {
            continue;
        }
        cpu += strlen(match_words);

        while ('\0' != cpu[0]) {
            if (' ' != cpu[0]) {
                cpu_id.push_back(cpu[0]);
            }
            ++cpu;
        }

        if (!cpu_id.empty()) {
            break;
        }
    }

    ifs.close();
}

static bool get_cpu_id_by_system(std::string &cpu_id)
{
    const char *dmidecode_result = ".dmidecode_result.txt";
    char command[512] = {0};
    snprintf(command, sizeof(command), "dmidecode -t processor | grep ID > %s", dmidecode_result);

    if (0 == system(command)) {
        parse_cpu_id(dmidecode_result, "ID:", cpu_id);
    }

    unlink(dmidecode_result);

    return (!cpu_id.empty());
}

bool get_cpu_id(std::string &cpu_id)
{
    if (get_cpu_id_by_asm(cpu_id)) {
        return (true);
    }
    if (0 == getuid()) {
        if (get_cpu_id_by_system(cpu_id)) {
            return (true);
        }
    }
    return (false);
}

static void parse_disk_name(const char *file_name, const char *match_words, std::string &disk_name)
{
    std::ifstream ifs(file_name, std::ios::binary);
    if (!ifs.is_open()) {
        return;
    }

    char line[4096] = {0};
    while (!ifs.eof()) {
        ifs.getline(line, sizeof(line));
        if (!ifs.good()) {
            break;
        }
        const char *board = strstr(line, match_words);
        if (NULL == board) {
            continue;
        }
        if (strlen(board) != 1) {
            continue;
        }
        break;
    }

    while (!ifs.eof()) {
        ifs.getline(line, sizeof(line));
        if (!ifs.good()) {
            break;
        }
        const char *board = strstr(line, "disk");
        if (NULL == board) {
            continue;
        }

        int i = 0;
        while (' ' != line[i]) {
            disk_name.push_back(line[i]);
            ++i;
        }
        if (!disk_name.empty()) {
            break;
        }
    }
    ifs.close();
}

static void parse_disk_serial(const char *file_name, const char *match_words, std::string &serial_no)
{
    std::ifstream ifs(file_name, std::ios::binary);
    if (!ifs.is_open()) {
        return;
    }

    char line[4096] = {0};
    while (!ifs.eof()) {
        ifs.getline(line, sizeof(line));
        if (!ifs.good()) {
            break;
        }

        const char *board = strstr(line, match_words);
        if (NULL == board) {
            continue;
        }
        board += strlen(match_words);

        while ('\0' != board[0]) {
            if (' ' != board[0]) {
                serial_no.push_back(board[0]);
            }
            ++board;
        }

        if ("None" == serial_no) {
            serial_no.clear();
            continue;
        }

        if (!serial_no.empty()) {
            break;
        }
    }

    ifs.close();
}

static bool get_disk_name_system(std::string &disk_name)
{
    const char *dmidecode_result = ".dmidecode_result.txt";
    char command[512] = {0};
    snprintf(command, sizeof(command), "lsblk -nls > %s", dmidecode_result);

    if (0 == system(command)) {
        parse_disk_name(dmidecode_result, "/", disk_name);
    }

    unlink(dmidecode_result);

    return (!disk_name.empty());
}

static bool get_disk_by_system(std::string disk_name, std::string &serial_no)
{
    const char *dmidecode_result = ".dmidecode_result.txt";
    char command[512] = {0};
    snprintf(command, sizeof(command), "udevadm info --query=all --name=%s | grep ID_SERIAL= > %s", disk_name.c_str(), dmidecode_result);

    if (0 == system(command)) {
        parse_disk_serial(dmidecode_result, "ID_SERIAL=", serial_no);
    }

    unlink(dmidecode_result);

    return (!serial_no.empty());
}

bool get_disk_id(std::string &disk_id)
{
    if (0 == getuid()) {
        std::string disk_name;
        if (get_disk_name_system(disk_name)) {
            if (get_disk_by_system(disk_name, disk_id)) {
                return (true);
            }
        }
    }
    return (false);
}

#endif
