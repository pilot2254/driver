#include <Windows.h>
#include <stdio.h>
#include "driver.hpp"

int main() {
	// After loading the driver via kdmapper, check DebugView for:
	// [km] request slot at 0xFFFF...
	// paste that address here
	uintptr_t requestSlot = 0xFFFFFFFFFFFFFFFF; // <-- replace with address from DebugView

	if (requestSlot == 0xFFFFFFFFFFFFFFFF) {
		printf("set requestSlot to the address printed by the driver in DebugView\n");
		return 1;
	}

	if (!DriverInit(requestSlot)) {
		printf("DriverInit failed\n");
		return 1;
	}

	printf("[+] driver initialized\n");

	// test 1: get process id
	DWORD pid = 0;
	if (GetProcessId(L"notepad.exe", pid)) {
		printf("[+] notepad.exe PID: %lu\n", pid);
	}
	else {
		printf("[-] notepad.exe not found (is it running?)\n");
		return 1;
	}

	// test 2: get module base
	uintptr_t base = 0;
	if (GetModuleBase(pid, "notepad.exe", base)) {
		printf("[+] notepad.exe base: 0x%llX\n", base);
	}
	else {
		printf("[-] failed to get module base\n");
	}

	// test 3: read memory (read first 4 bytes of notepad — should be MZ header 0x5A4D)
	if (base) {
		uint16_t mz = 0;
		if (ReadMemory<uint16_t>(pid, base, mz)) {
			printf("[+] first 2 bytes at base: 0x%X (should be 0x5A4D)\n", mz);
		}
		else {
			printf("[-] read failed\n");
		}
	}

	printf("[+] all tests done\n");
	return 0;
}
