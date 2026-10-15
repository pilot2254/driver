#include <ntddk.h>

//forward declarations
DRIVER_UNLOAD DriverUnload;
DRIVER_INITIALIZE DriverEntry;


//DriverUnload is called when someone runs:
//sc stop WdmHello

//without this, windows refuses to unload this driver
//always implement this during dev

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("[WdmMike] Unloaded\n");
}


//DriverEntry = "main"
//called once when the driver loads

//DriverObject = kernels representation of this driver
//RegistryPath = HKLM\SYSTEM\CurrentControlSet\Services\WdmHello

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	//tell Windows how to unload
	DriverObject->DriverUnload = DriverUnload;

	DbgPrint("[WdmMike] Loaded successfully\n");

	return STATUS_SUCCESS;
}
