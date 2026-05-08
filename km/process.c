#include "driver.h"

NTSTATUS KmGetProcessByName(PWCH Name, PULONG OutPid) {
	if (!Name || !OutPid)
		return STATUS_INVALID_PARAMETER;

	*OutPid = 0;
	ULONG bufSize = 0;

	// first call to get required size
	NTSTATUS status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &bufSize);
	if (status != STATUS_INFO_LENGTH_MISMATCH)
		return status;

	PVOID buf = ExAllocatePool2(POOL_FLAG_NON_PAGED, bufSize, 'corP');
	if (!buf)
		return STATUS_INSUFFICIENT_RESOURCES;

	status = ZwQuerySystemInformation(SystemProcessInformation, buf, bufSize, &bufSize);
	if (!NT_SUCCESS(status)) {
		ExFreePoolWithTag(buf, 'corP');
		return status;
	}

	PSYSTEM_PROCESS_INFORMATION entry = (PSYSTEM_PROCESS_INFORMATION)buf;
	status = STATUS_NOT_FOUND;

	while (TRUE) {
		if (entry->ImageName.Buffer && _wcsicmp(entry->ImageName.Buffer, Name) == 0) {
			*OutPid = (ULONG)(ULONG_PTR)entry->UniqueProcessId;
			status = STATUS_SUCCESS;
			break;
		}

		if (entry->NextEntryOffset == 0)
			break;

		entry = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)entry + entry->NextEntryOffset);
	}

	ExFreePoolWithTag(buf, 'corP');
	return status;
}

NTSTATUS KmGetModuleBase(ULONG ProcessId, PCHAR ModuleName, PULONG64 OutBase) {
	if (!ModuleName || !OutBase)
		return STATUS_INVALID_PARAMETER;

	*OutBase = 0;

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)ProcessId, &process);
	if (!NT_SUCCESS(status))
		return status;

	KAPC_STATE apc;
	KeStackAttachProcess(process, &apc);

	__try {
		PPEB peb = PsGetProcessPeb(process);
		if (!peb || !peb->Ldr) {
			status = STATUS_NOT_FOUND;
			__leave;
		}

		PLIST_ENTRY head = &peb->Ldr->InMemoryOrderModuleList;
		PLIST_ENTRY curr = head->Flink;

		while (curr != head) {
			PLDR_DATA_TABLE_ENTRY mod = CONTAINING_RECORD(curr, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

			if (mod->FullDllName.Buffer) {
				// convert module name to compare (find just the filename part)
				UNICODE_STRING target;
				ANSI_STRING ansiTarget;
				RtlInitAnsiString(&ansiTarget, ModuleName);
				status = RtlAnsiStringToUnicodeString(&target, &ansiTarget, TRUE);

				if (NT_SUCCESS(status)) {
					// get just the filename from full path
					UNICODE_STRING fileName = mod->FullDllName;
					for (int i = fileName.Length / 2 - 1; i >= 0; i--) {
						if (fileName.Buffer[i] == L'\\') {
							fileName.Buffer = &fileName.Buffer[i + 1];
							fileName.Length -= (USHORT)((i + 1) * 2);
							break;
						}
					}

					if (RtlEqualUnicodeString(&fileName, &target, TRUE)) {
						*OutBase = (ULONG64)mod->DllBase;
						RtlFreeUnicodeString(&target);
						status = STATUS_SUCCESS;
						__leave;
					}

					RtlFreeUnicodeString(&target);
				}
			}

			curr = curr->Flink;
		}

		status = STATUS_NOT_FOUND;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		status = GetExceptionCode();
	}

	KeUnstackDetachProcess(&apc);
	ObDereferenceObject(process);
	return status;
}
