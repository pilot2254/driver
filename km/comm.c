#include "driver.h"

VOID CommHandleRequest(PCOMM_REQUEST Req) {
	if (!Req || Req->Magic != COMM_MAGIC) {
		if (Req) Req->Status = STATUS_INVALID_PARAMETER;
		return;
	}

	switch (Req->Operation) {
	case OP_READ_MEMORY:
		Req->Status = KmReadMemory(Req->ProcessId, Req->Address, Req->Buffer, (SIZE_T)Req->Size);
		break;

	case OP_WRITE_MEMORY:
		Req->Status = KmWriteMemory(Req->ProcessId, Req->Address, Req->Buffer, (SIZE_T)Req->Size);
		break;

	case OP_GET_PROCESS:
		Req->Status = KmGetProcessByName(Req->ProcessName, &Req->ProcessId);
		break;

	case OP_GET_MODULE:
		Req->Status = KmGetModuleBase(Req->ProcessId, Req->ModuleName, &Req->ModuleBase);
		break;

	default:
		Req->Status = STATUS_INVALID_PARAMETER;
		break;
	}
}
