#include "driver.h"

// original function pointer
static SHORT(NTAPI* OrigNtUserGetAsyncKeyState)(INT vKey) = NULL;

// our hook — UM passes a pointer to COMM_REQUEST cast to int
// on x64 we can't fit a 64bit pointer in an int, so we use
// a global slot approach: UM writes to g_Request, then calls
// GetAsyncKeyState with COMM_MAGIC to trigger handling
static volatile PCOMM_REQUEST g_Request = NULL;
static volatile LONG g_Busy = 0;

SHORT NTAPI HookedNtUserGetAsyncKeyState(INT vKey) {
	if ((ULONG)vKey == COMM_MAGIC) {
		// spin until we get the lock
		if (InterlockedCompareExchange(&g_Busy, 1, 0) == 0) {
			PCOMM_REQUEST req = (PCOMM_REQUEST)g_Request;
			if (req) {
				CommHandleRequest(req);
				g_Request = NULL;
			}
			InterlockedExchange(&g_Busy, 0);
		}
		return 0;
	}

	return OrigNtUserGetAsyncKeyState(vKey);
}

// find a function by walking exports of a module
static PVOID GetKernelExport(PVOID base, const char* name) {
	if (!base || !name) return NULL;

	__try {
		PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
		if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;

		PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((PUCHAR)base + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;

		IMAGE_DATA_DIRECTORY expDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		if (!expDir.VirtualAddress) return NULL;

		PIMAGE_EXPORT_DIRECTORY exp = (PIMAGE_EXPORT_DIRECTORY)((PUCHAR)base + expDir.VirtualAddress);
		PULONG  names = (PULONG)((PUCHAR)base + exp->AddressOfNames);
		PUSHORT ords = (PUSHORT)((PUCHAR)base + exp->AddressOfNameOrdinals);
		PULONG  funcs = (PULONG)((PUCHAR)base + exp->AddressOfFunctions);

		for (ULONG i = 0; i < exp->NumberOfNames; i++) {
			const char* exportName = (const char*)((PUCHAR)base + names[i]);
			if (strcmp(exportName, name) == 0)
				return (PUCHAR)base + funcs[ords[i]];
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}

	return NULL;
}

// get win32k.sys base from loaded module list
static PVOID GetWin32kBase() {
	PLIST_ENTRY head = &PsLoadedModuleList;
	if (!head) return NULL;

	for (PLIST_ENTRY entry = head->Flink; entry != head; entry = entry->Flink) {
		PKLDR_DATA_TABLE_ENTRY mod = CONTAINING_RECORD(entry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (mod->BaseDllName.Buffer &&
			_wcsicmp(mod->BaseDllName.Buffer, L"win32kbase.sys") == 0) {
			return mod->DllBase;
		}
	}
	return NULL;
}

// write a JMP hook at target, save original bytes + trampoline
#define HOOK_SIZE 14 // MOV RAX, addr; JMP RAX

static UCHAR g_OrigBytes[HOOK_SIZE] = { 0 };
static PVOID g_HookTarget = NULL;

static BOOLEAN WriteHook(PVOID target, PVOID hookFn) {
	// disable write protection
	UCHAR patch[HOOK_SIZE] = {
	    0x48, 0xB8,                         // MOV RAX, imm64
	    0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,             // (address filled below)
	    0xFF, 0xE0,                         // JMP RAX
	    0x90, 0x90                          // NOP padding
	};

	*(PVOID*)(patch + 2) = hookFn;

	// save original bytes for unhook
	RtlCopyMemory(g_OrigBytes, target, HOOK_SIZE);
	g_HookTarget = target;

	// make page writable via MDL
	PMDL mdl = IoAllocateMdl(target, HOOK_SIZE, FALSE, FALSE, NULL);
	if (!mdl) return FALSE;

	MmBuildMdlForNonPagedPool(mdl);
	mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

	PVOID mapped = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
	if (!mapped) {
		IoFreeMdl(mdl);
		return FALSE;
	}

	RtlCopyMemory(mapped, patch, HOOK_SIZE);

	MmUnmapLockedPages(mapped, mdl);
	IoFreeMdl(mdl);

	return TRUE;
}

static VOID RemoveHook() {
	if (!g_HookTarget) return;

	PMDL mdl = IoAllocateMdl(g_HookTarget, HOOK_SIZE, FALSE, FALSE, NULL);
	if (!mdl) return;

	MmBuildMdlForNonPagedPool(mdl);
	mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

	PVOID mapped = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
	if (mapped) {
		RtlCopyMemory(mapped, g_OrigBytes, HOOK_SIZE);
		MmUnmapLockedPages(mapped, mdl);
	}

	IoFreeMdl(mdl);
	g_HookTarget = NULL;
}

// exported so UM can find it by scanning — this is how UM submits requests
// UM scans for this export in driver memory and writes to it directly
PVOID g_RequestSlot = NULL; // UM writes PCOMM_REQUEST here, then triggers via GetAsyncKeyState

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrint("[km] loading\n");

	PVOID win32k = GetWin32kBase();
	if (!win32k) {
		DbgPrint("[km] win32kbase.sys not found\n");
		return STATUS_NOT_FOUND;
	}

	PVOID target = GetKernelExport(win32k, "NtUserGetAsyncKeyState");
	if (!target) {
		DbgPrint("[km] NtUserGetAsyncKeyState not found\n");
		return STATUS_NOT_FOUND;
	}

	OrigNtUserGetAsyncKeyState = (SHORT(NTAPI*)(INT))target;

	if (!WriteHook(target, HookedNtUserGetAsyncKeyState)) {
		DbgPrint("[km] hook failed\n");
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("[km] hooked NtUserGetAsyncKeyState at %p\n", target);
	DbgPrint("[km] request slot at %p\n", &g_Request);
	DbgPrint("[km] loaded\n");

	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	RemoveHook();
	DbgPrint("[km] unloaded\n");
}
