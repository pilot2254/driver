#include "driver.h"

PDEVICE_OBJECT g_DeviceObject = NULL;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\MemDriver");
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\MemDriver");

	DbgPrint("[driver] Loading...\n");

	//create device
	status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&g_DeviceObject
	);

	if (!NT_SUCCESS(status)) {
		DbgPrint("[driver] IoCreateDevice failed: 0x%X\n", status);
		return status;
	}

	//create symbolic link
	status = IoCreateSymbolicLink(&symLink, &deviceName);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[driver] IoCreateSymbolicLink failed: 0x%X\n", status);
		IoDeleteDevice(g_DeviceObject);
		return status;
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;

	DbgPrint("[driver] Loaded successfully\n");
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\MemDriver");

	DbgPrint("[driver] Unloading...\n");

	IoDeleteSymbolicLink(&symLink);
	if (g_DeviceObject) {
		IoDeleteDevice(g_DeviceObject);
	}

	UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS status = STATUS_SUCCESS;
	ULONG bytesReturned = 0;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;

	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
	ULONG inSize = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outSize = stack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (controlCode) {
	case IOCTL_READ_MEMORY: {
		if (inSize < sizeof(READ_REQUEST) || outSize < sizeof(READ_REQUEST)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		PREAD_REQUEST req = (PREAD_REQUEST)buffer;

		if (req->Size == 0 || req->Size > sizeof(req->Data)) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = ReadProcessMemory(req->ProcessId, req->Address, req->Data, req->Size);
		if (NT_SUCCESS(status))
			bytesReturned = sizeof(READ_REQUEST);
		break;
	}

	case IOCTL_WRITE_MEMORY: {
		if (inSize < sizeof(WRITE_REQUEST)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		PWRITE_REQUEST req = (PWRITE_REQUEST)buffer;

		if (req->Size == 0 || inSize < sizeof(WRITE_REQUEST) + req->Size - 1) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = WriteProcessMemory(req->ProcessId, req->Address, req->Buffer, req->Size);
		break;
	}

	case IOCTL_GET_PROCESS: {
		if (inSize < sizeof(PROCESS_REQUEST) || outSize < sizeof(PROCESS_REQUEST)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		PPROCESS_REQUEST req = (PPROCESS_REQUEST)buffer;
		status = GetProcessIdByName(req->ProcessName, &req->ProcessId);
		if (NT_SUCCESS(status)) {
			bytesReturned = sizeof(PROCESS_REQUEST);
		}
		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = bytesReturned;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}
