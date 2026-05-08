#include "driver.h"

// original NtUserGetAsyncKeyState pointer
static SHORT(NTAPI* OriginalGetAsyncKeyState)(INT vKey) = NULL;

// our hook
SHORT NTAPI HookedGetAsyncKeyState(INT vKey) {
	// check if it's our magic value
	if ((ULONG)vKey == COMM_MAGIC) {
		// the "key" is actually a pointer to our request struct
		// UM passes it as (INT)(ULONG_PTR)&request
		// we can't do that in 64bit — see communication.c for the proper approach
		return 0;
	}
	return OriginalGetAsyncKeyState(vKey);
}

// find export in a module
PVOID FindExport(PVOID moduleBase, const char* exportName) {
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)moduleBase;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((PUCHAR)moduleBase + dos->e_lfanew);
	PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)(
		(PUCHAR)moduleBase + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
		);

	PULONG names = (PULONG)((PUCHAR)moduleBase + exports->AddressOfNames);
	PUSHORT ordinals = (PUSHORT)((PUCHAR)moduleBase + exports->AddressOfNameOrdinals);
	PULONG functions = (PULONG)((PUCHAR)moduleBase + exports->AddressOfFunctions);

	for (ULONG i = 0; i < exports->NumberOfNames; i++) {
		const char* name = (const char*)((PUCHAR)moduleBase + names[i]);
		if (strcmp(name, exportName) == 0) {
			return (PUCHAR)moduleBase + functions[ordinals[i]];
		}
	}
	return NULL;
}

// get win32k base from loaded module list
PVOID GetWin32kBase() {
	UNICODE_STRING name = RTL_CONSTANT_STRING(L"win32k.sys");
	return RtlFindExportedRoutineByName(PsLoadedModuleList, "NtUserGetAsyncKeyState");
}

// handle request from UM
VOID HandleRequest(PCOMM_REQUEST req) {
	if (!req || req->Magic != COMM_MAGIC) return;

	switch (req->Operation) {
	case OP_READ_MEMORY:
		req->Status = ReadProcessMemory(req->ProcessId, req->Address, req->Buffer, req->Size);
		break;
	case OP_WRITE_MEMORY:
		req->Status = WriteProcessMemory(req->ProcessId, req->Address, req->Buffer, req->Size);
		break;
	case OP_GET_PROCESS:
		req->Status = GetProcessIdByName(req->ProcessName, &req->ProcessId);
		break;
	default:
		req->Status = STATUS_INVALID_PARAMETER;
		break;
	}
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrint("[driver] Loaded\n");

	// nothing to init here — communication.c handles the hook setup
	return STATUS_SUCCESS;
}
