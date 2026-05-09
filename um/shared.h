#pragma once

#define OP_READ_MEMORY  0x10
#define OP_WRITE_MEMORY 0x11
#define OP_GET_PROCESS  0x12
#define OP_GET_MODULE   0x13

#define COMM_MAGIC 0xCAFEBABE

#pragma pack(push, 1)
typedef struct _COMM_REQUEST {
	unsigned long  Magic;
	unsigned long  Operation;
	unsigned long  ProcessId;
	void* Address;
	void* Buffer;
	unsigned long long Size;
	wchar_t        ProcessName[260];
	char           ModuleName[260];
	unsigned long long ModuleBase;
	long           Status;
} COMM_REQUEST, * PCOMM_REQUEST;
#pragma pack(pop)
