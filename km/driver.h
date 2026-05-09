#pragma once

#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <wdm.h>
#include <ntimage.h>
#include "shared.h"

#define MAX_RW_SIZE 0x1000000 // 16mb

// undocumented — needed for MmCopyVirtualMemory
NTKERNELAPI NTSTATUS MmCopyVirtualMemory(
	PEPROCESS SourceProcess,
	PVOID SourceAddress,
	PEPROCESS TargetProcess,
	PVOID TargetAddress,
	SIZE_T BufferSize,
	KPROCESSOR_MODE PreviousMode,
	PSIZE_T ReturnSize
);

// PsLoadedModuleList
extern NTKERNELAPI LIST_ENTRY PsLoadedModuleList;

// kernel module list entry
typedef struct _KLDR_DATA_TABLE_ENTRY {
	LIST_ENTRY     InLoadOrderLinks;
	PVOID          ExceptionTable;
	ULONG          ExceptionTableSize;
	PVOID          GpValue;
	PVOID          NonPagedDebugInfo;
	PVOID          DllBase;
	PVOID          EntryPoint;
	ULONG          SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG          Flags;
	USHORT         LoadCount;
	USHORT         TlsIndex;
} KLDR_DATA_TABLE_ENTRY, * PKLDR_DATA_TABLE_ENTRY;

// usermode PEB module list entry
typedef struct _LDR_MODULE {
	LIST_ENTRY     InLoadOrderModuleList;
	LIST_ENTRY     InMemoryOrderModuleList;
	LIST_ENTRY     InInitializationOrderModuleList;
	PVOID          DllBase;
	PVOID          EntryPoint;
	ULONG          SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG          Flags;
	USHORT         LoadCount;
	USHORT         TlsIndex;
} LDR_MODULE, * PLDR_MODULE;

typedef struct _PEB_LDR_DATA {
	ULONG      Length;
	BOOLEAN    Initialized;
	PVOID      SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _MY_PEB {
	UCHAR         Reserved1[2];
	UCHAR         BeingDebugged;
	UCHAR         Reserved2[1];
	PVOID         Reserved3[2];
	PPEB_LDR_DATA Ldr;
} MY_PEB, * PMY_PEB;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG          NextEntryOffset;
	ULONG          NumberOfThreads;
	LARGE_INTEGER  Reserved1[3];
	LARGE_INTEGER  CreateTime;
	LARGE_INTEGER  UserTime;
	LARGE_INTEGER  KernelTime;
	UNICODE_STRING ImageName;
	KPRIORITY      BasePriority;
	HANDLE         UniqueProcessId;
	HANDLE         InheritedFromUniqueProcessId;
	ULONG          HandleCount;
	ULONG          SessionId;
	ULONG_PTR      PageDirectoryBase;
	SIZE_T         PeakVirtualSize;
	SIZE_T         VirtualSize;
	ULONG          PageFaultCount;
	SIZE_T         PeakWorkingSetSize;
	SIZE_T         WorkingSetSize;
	SIZE_T         QuotaPeakPagedPoolUsage;
	SIZE_T         QuotaPagedPoolUsage;
	SIZE_T         QuotaPeakNonPagedPoolUsage;
	SIZE_T         QuotaNonPagedPoolUsage;
	SIZE_T         PagefileUsage;
	SIZE_T         PeakPagefileUsage;
	SIZE_T         PrivatePageCount;
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemProcessInformation = 5
} SYSTEM_INFORMATION_CLASS;

NTKERNELAPI NTSTATUS ZwQuerySystemInformation(
	IN  SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID                    SystemInformation,
	IN  ULONG                    SystemInformationLength,
	OUT PULONG                   ReturnLength OPTIONAL
);

// memory.c
NTSTATUS KmReadMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size);
NTSTATUS KmWriteMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size);

// process.c
NTSTATUS KmGetProcessByName(PWCH Name, PULONG OutPid);
NTSTATUS KmGetModuleBase(ULONG ProcessId, PCHAR ModuleName, PULONG64 OutBase);

// comm.c
VOID CommHandleRequest(PCOMM_REQUEST Req);
