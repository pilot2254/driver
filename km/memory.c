#include "driver.h"

#define MAX_RW_SIZE 0x1000000 // 16MB cap, sanity check

// Attach to target process
NTSTATUS AttachToProcess(ULONG ProcessId, PEPROCESS* Process) {
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)ProcessId, Process);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[driver] Failed to find process: %lu\n", ProcessId);
		return status;
	}
	return STATUS_SUCCESS;
}

// Read memory from target process
NTSTATUS ReadProcessMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size) {
	if (!Address || !Buffer || Size == 0 || Size > MAX_RW_SIZE)
	{
		return STATUS_INVALID_PARAMETER;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = AttachToProcess(ProcessId, &process);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	// Attach to process address space
	KAPC_STATE apcState;
	KeStackAttachProcess(process, &apcState);

	// Probe and read
	__try {
		ProbeForRead(Address, Size, 1);
		RtlCopyMemory(Buffer, Address, Size);
		status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		status = GetExceptionCode();
		DbgPrint("[driver] Read exception: 0x%X\n", status);
	}

	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(process);

	return status;
}

// Write memory to target process
NTSTATUS WriteProcessMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size) {
	if (!Address || !Buffer || Size == 0) {
		return STATUS_INVALID_PARAMETER;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = AttachToProcess(ProcessId, &process);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	KAPC_STATE apcState;
	KeStackAttachProcess(process, &apcState);

	__try {
		ProbeForWrite(Address, Size, 1);
		RtlCopyMemory(Address, Buffer, Size);
		status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		status = GetExceptionCode();
		DbgPrint("[driver] Write exception: 0x%X\n", status);
	}

	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(process);

	return status;
}

// Get process ID by name
NTSTATUS GetProcessIdByName(PWCH ProcessName, PULONG ProcessId) {
	if (!ProcessName || !ProcessId) {
		return STATUS_INVALID_PARAMETER;
	}

	*ProcessId = 0;
	NTSTATUS status = STATUS_NOT_FOUND;
	PVOID buffer = NULL;
	ULONG bufferSize = 0;

	// Query system information
	status = ZwQuerySystemInformation(SystemProcessInformation, buffer, bufferSize, &bufferSize);
	if (status != STATUS_INFO_LENGTH_MISMATCH) {
		return status;
	}

	buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, bufferSize, 'prcS');
	if (!buffer) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = ZwQuerySystemInformation(SystemProcessInformation, buffer, bufferSize, &bufferSize);
	if (!NT_SUCCESS(status)) {
		ExFreePoolWithTag(buffer, 'prcS');
		return status;
	}

	PSYSTEM_PROCESS_INFORMATION processInfo = (PSYSTEM_PROCESS_INFORMATION)buffer;

	// Iterate through processes
	while (TRUE) {
		if (processInfo->ImageName.Buffer != NULL) {
			if (_wcsicmp(processInfo->ImageName.Buffer, ProcessName) == 0) {
				*ProcessId = (ULONG)(ULONG_PTR)processInfo->UniqueProcessId;
				status = STATUS_SUCCESS;
				break;
			}
		}

		if (processInfo->NextEntryOffset == 0) {
			break;
		}

		processInfo = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)processInfo + processInfo->NextEntryOffset);
	}

	ExFreePoolWithTag(buffer, 'prcS');
	return status;
}
