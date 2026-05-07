#include <Windows.h>
#include <stdio.h>

//same codes as driver.h
#define IOCTL_BASE 0x800
#define IOCTL_GET_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE + 0x3, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
	WCHAR ProcessName[260];
	ULONG ProcessId;
} PROCESS_REQUEST;

int main() {
	HANDLE device = CreateFileW(
		L"\\\\.\\MemDriver",
		GENERIC_READ | GENERIC_WRITE,
		0, NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (device == INVALID_HANDLE_VALUE) {
		printf("Failed to open device: %lu\n", GetLastError());
		return 1;
	}

	PROCESS_REQUEST req = { 0 };
	wcscpy_s(req.ProcessName, 260, L"notepad.exe");

	DWORD bytesReturned = 0;
	BOOL ok = DeviceIoControl(device, IOCTL_GET_PROCESS, &req, sizeof(req), &req, sizeof(req), &bytesReturned, NULL);

	if (ok)
		printf("notepad.exe PID: %lu\n", req.ProcessId);
	else
		printf("IOCTL failed: %lu\n", GetLastError());

	CloseHandle(device);
	return 0;
}
