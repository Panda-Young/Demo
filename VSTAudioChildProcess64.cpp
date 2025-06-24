#include <iomanip> // For std::hex and std::setw
#include <iostream>
#include <vector>
#include <windows.h>

#define PIPE_NAME L"\\\\.\\pipe\\VSTAudioPipe"
#define EVENT_TO_CHILD_NAME L"Global\\VST_Event_To_Child"
#define EVENT_TO_PARENT_NAME L"Global\\VST_Event_To_Parent"

// Helper macro for logging with function name and line number
#define LOG(msg) std::cout << "[" << __FILE__ << ":" << std::dec << __LINE__ << "] " << msg << std::endl

int main()
{
    LOG("Starting VST Audio Child Process...");

    // Create named pipe server
    LOG("Creating named pipe server...");
    HANDLE hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, // max instances
        1024 * 1024, 1024 * 1024, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        LOG("Failed to create named pipe! Error: " << std::hex << GetLastError());
        return 1;
    }
    LOG("Named pipe created successfully");

    // Open events
    LOG("Opening synchronization events...");
    HANDLE hEventToChild = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_TO_CHILD_NAME);
    HANDLE hEventToParent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_TO_PARENT_NAME);

    if (!hEventToChild || !hEventToParent) {
        DWORD err = GetLastError();
        LOG("Failed to open events! Error Code: "
            << std::dec << err
            << " (" << (err == ERROR_ACCESS_DENIED ? "Access Denied" : err == ERROR_FILE_NOT_FOUND ? "Event Not Found"
                                                                                                   : "Unknown")
            << ")");
        return 2;
    }
    LOG("Synchronization events opened successfully");

    // Wait for connection from parent
    LOG("Waiting for parent process connection...");
    if (!ConnectNamedPipe(hPipe, NULL) && GetLastError() != ERROR_PIPE_CONNECTED) {
        LOG("Pipe connection failed! Error: " << std::hex << GetLastError());
        return 3;
    }
    LOG("Connected to parent process");

    while (true) {
        // LOG("Waiting for data notification from parent...");
        DWORD waitRes = WaitForSingleObject(hEventToChild, INFINITE);

        if (waitRes != WAIT_OBJECT_0) {
            LOG("Wait for event failed with code: " << std::hex << waitRes);
            break;
        }

        // LOG("Event received, processing data...");

        // Read [N][C] and data
        int N = 0, C = 0;
        DWORD nBytes = 0;

        if (!ReadFile(hPipe, &N, sizeof(int), &nBytes, NULL)) {
            LOG("Failed to read N! Error: " << std::hex << GetLastError());
            break;
        }
        //LOG("Read N = " << N);

        if (!ReadFile(hPipe, &C, sizeof(int), &nBytes, NULL)) {
            LOG("Failed to read C! Error: " << std::hex << GetLastError());
            break;
        }
        // LOG("Read C = " << C);

        int total = N * C;
        std::vector<float> data(total);

        if (!ReadFile(hPipe, data.data(), total * sizeof(float), &nBytes, NULL)) {
            LOG("Failed to read data buffer! Error: " << std::hex << GetLastError());
            break;
        }
        // LOG("Successfully read " << total << " float values");

        // Example: apply gain
        // LOG("Processing audio data (applying gain)...");
        for (int i = 0; i < total; ++i)
            data[i] *= 0.175f;

        // Write back processed data
        // LOG("Writing processed data back to pipe...");
        if (!WriteFile(hPipe, data.data(), total * sizeof(float), &nBytes, NULL)) {
            LOG("Failed to write data! Error: " << std::hex << GetLastError());
            break;
        }
        // LOG("Successfully wrote " << total << " float values");

        SetEvent(hEventToParent);
        // LOG("Notified parent process of completion");
    }

    LOG("Cleaning up resources...");
    CloseHandle(hPipe);
    CloseHandle(hEventToChild);
    CloseHandle(hEventToParent);
    LOG("Process terminating");
    return 0;
}
