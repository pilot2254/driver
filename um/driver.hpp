#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>
#include "shared.h"

// How UM communicates with KM:
// 1. Find the driver's g_Request global by scanning kernel memory (via NtQuerySystemInformation + MmMapLockedPages trick)
//    OR for testing: kdmapper prints the address via DbgPrint, read it from DebugView
// 2. Write &req to g_Request
// 3. Call NtUserGetAsyncKeyState(COMM_MAGIC) to trigger the handler
// 4. Read req.Status for result

using NtUserGetAsyncKeyState_t = SHORT(NTAPI*)(INT);
static NtUserGetAsyncKeyState_t pNtUserGetAsyncKeyState = nullptr;

// address of g_Request in kernel — set this from DebugView output during testing
// in production you'd scan for it
static volatile COMM_REQUEST** g_KernelRequestSlot = nullptr;

inline bool DriverInit(uintptr_t requestSlotKernelAddr) {
	// get NtUserGetAsyncKeyState from win32u.dll
	HMODULE win32u = LoadLibraryW(L"win32u.dll");
	if (!win32u) return false;

	pNtUserGetAsyncKeyState = (NtUserGetAsyncKeyState_t)GetProcAddress(win32u, "NtUserGetAsyncKeyState");
	if (!pNtUserGetAsyncKeyState) return false;

	g_KernelRequestSlot = (volatile COMM_REQUEST**)requestSlotKernelAddr;
	return true;
}

inline long SendRequest(COMM_REQUEST& req) {
	if (!pNtUserGetAsyncKeyState || !g_KernelRequestSlot)
		return -1;

	req.Magic = COMM_MAGIC;

	// write request pointer to kernel slot
	// NOTE: this requires a way to write to kernel memory from UM
	// use VirtualAllocEx + WriteProcessMemory into kernel, or map shared memory
	// for now we use a kernel-mode mapped shared section (see below)
	*g_KernelRequestSlot = &req;

	// trigger the hook
	pNtUserGetAsyncKeyState((INT)COMM_MAGIC);

	// spin until km clears the slot (means it's done)
	while (*g_KernelRequestSlot != nullptr)
		Sleep(0);

	return req.Status;
}

// --- high level API ---

inline bool GetProcessId(const wchar_t* name, DWORD& outPid) {
	COMM_REQUEST req = {};
	req.Operation = OP_GET_PROCESS;
	wcscpy_s(req.ProcessName, name);
	outPid = 0;

	if (SendRequest(req) != 0) return false;
	outPid = req.ProcessId;
	return true;
}

inline bool GetModuleBase(DWORD pid, const char* moduleName, uintptr_t& outBase) {
	COMM_REQUEST req = {};
	req.Operation = OP_GET_MODULE;
	req.ProcessId = pid;
	strcpy_s(req.ModuleName, moduleName);
	outBase = 0;

	if (SendRequest(req) != 0) return false;
	outBase = (uintptr_t)req.ModuleBase;
	return true;
}

template<typename T>
inline bool ReadMemory(DWORD pid, uintptr_t address, T& outValue) {
	COMM_REQUEST req = {};
	req.Operation = OP_READ_MEMORY;
	req.ProcessId = pid;
	req.Address = (void*)address;
	req.Buffer = &outValue;
	req.Size = sizeof(T);

	return SendRequest(req) == 0;
}

template<typename T>
inline bool WriteMemory(DWORD pid, uintptr_t address, const T& value) {
	COMM_REQUEST req = {};
	req.Operation = OP_WRITE_MEMORY;
	req.ProcessId = pid;
	req.Address = (void*)address;
	req.Buffer = (void*)&value;
	req.Size = sizeof(T);

	return SendRequest(req) == 0;
}
