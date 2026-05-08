#include "driver.h"

// UM writes the request VA here, driver polls and handles it
static volatile PCOMM_REQUEST g_PendingRequest = NULL;
static KSPIN_LOCK g_Lock;
static BOOLEAN g_Running = FALSE;
static HANDLE g_ThreadHandle = NULL;

VOID CommThread(PVOID context) {
	UNREFERENCED_PARAMETER(context);

	while (g_Running) {
		PCOMM_REQUEST req = (PCOMM_REQUEST)InterlockedExchangePointer(
			(volatile PVOID*)&g_PendingRequest, NULL
		);

		if (req) {
			HandleRequest(req);
		}

		LARGE_INTEGER interval;
		interval.QuadPart = -1000; // 0.1ms
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS CommInit() {
	KeInitializeSpinLock(&g_Lock);
	g_Running = TRUE;

	NTSTATUS status = PsCreateSystemThread(
		&g_ThreadHandle, THREAD_ALL_ACCESS,
		NULL, NULL, NULL, CommThread, NULL
	);

	if (!NT_SUCCESS(status)) {
		g_Running = FALSE;
		DbgPrint("[driver] Failed to create comm thread: 0x%X\n", status);
	}

	return status;
}

VOID CommShutdown() {
	g_Running = FALSE;
	if (g_ThreadHandle) {
		ZwClose(g_ThreadHandle);
		g_ThreadHandle = NULL;
	}
}

// UM calls this by writing the request pointer to a known exported symbol
VOID CommSubmit(PCOMM_REQUEST req) {
	InterlockedExchangePointer((volatile PVOID*)&g_PendingRequest, req);
}
