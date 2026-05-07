#pragma once

#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>

// System structures
typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER Reserved1[3];
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ImageName;
	KPRIORITY BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
	ULONG HandleCount;
	ULONG SessionId;
	ULONG_PTR PageDirectoryBase;
	SIZE_T PeakVirtualSize;
	SIZE_T VirtualSize;
	ULONG PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemProcessInformation = 5
} SYSTEM_INFORMATION_CLASS;

NTKERNELAPI NTSTATUS ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

// IOCTL codes
#define IOCTL_BASE 0x800
#define IOCTL_READ_MEMORY  CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE + 0x1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE + 0x2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_PROCESS  CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE + 0x3, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Request structures
typedef struct _READ_REQUEST {
	ULONG ProcessId;
	PVOID Address;
	SIZE_T Size;
	BYTE Data[4096]; // max read size, adjust as needed
} READ_REQUEST, * PREAD_REQUEST;

typedef struct _WRITE_REQUEST {
	ULONG ProcessId;
	PVOID Address;
	SIZE_T Size;
	BYTE Buffer[1]; // variable length, data follows the struct
} WRITE_REQUEST, * PWRITE_REQUEST;

typedef struct _PROCESS_REQUEST {
	WCHAR ProcessName[260];
	ULONG ProcessId;
} PROCESS_REQUEST, * PPROCESS_REQUEST;

// Driver functions
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// Memory operations
NTSTATUS ReadProcessMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size);
NTSTATUS WriteProcessMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size);
NTSTATUS GetProcessIdByName(PWCH ProcessName, PULONG ProcessId);
