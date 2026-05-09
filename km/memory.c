#include "driver.h"

NTSTATUS KmReadMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size) {
	if (!Address || !Buffer || Size == 0 || Size > MAX_RW_SIZE)
		return STATUS_INVALID_PARAMETER;

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)ProcessId, &process);
	if (!NT_SUCCESS(status))
		return status;

	SIZE_T bytes = 0;
	status = MmCopyVirtualMemory(process, Address, PsGetCurrentProcess(), Buffer, Size, KernelMode, &bytes);

	ObDereferenceObject(process);
	return status;
}

NTSTATUS KmWriteMemory(ULONG ProcessId, PVOID Address, PVOID Buffer, SIZE_T Size) {
	if (!Address || !Buffer || Size == 0 || Size > MAX_RW_SIZE)
		return STATUS_INVALID_PARAMETER;

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)ProcessId, &process);
	if (!NT_SUCCESS(status))
		return status;

	SIZE_T bytes = 0;
	status = MmCopyVirtualMemory(PsGetCurrentProcess(), Buffer, process, Address, Size, KernelMode, &bytes);

	ObDereferenceObject(process);
	return status;
}
