# kdriver

kernel driver loaded through [kdmapper](https://github.com/TheCruZ/kdmapper)

gives a usermode client basic kernel comms for:

- reading memory
- writing memory
- finding processes
- getting module bases

no device object
no IOCTLs
just a cursed hook in `win32kbase.sys`

> [!WARNING]
> this is mostly research code rn. not stealthy. not safe. not production ready. run it in a VM

# how it works

## communication

instead of exposing a device and talking through IOCTLs like normal people do, this thing hijacks `NtUserGetAsyncKeyState` and uses it as a syscall bridge between UM and KM.

flow looks like this:

1. driver patches `NtUserGetAsyncKeyState` with a 14 byte trampoline (`mov rax + jmp rax`)
2. patch gets written through MDL remapping so write protection doesnt matter
3. usermode grabs the `g_Request` pointer from DebugView output
4. UM writes a `COMM_REQUEST*` into the slot
5. UM calls `NtUserGetAsyncKeyState(COMM_MAGIC)` through `win32u.dll`
6. hook catches it, processes request, clears slot
7. UM waits until slot becomes `NULL`
8. result gets read from `req.Status`

so yh this isnt "using" win32kbase. its literally patching live kernel code.

# supported ops

| op                | description                                               |
| ----------------- | --------------------------------------------------------- |
| `OP_READ_MEMORY`  | reads target memory with `MmCopyVirtualMemory`            |
| `OP_WRITE_MEMORY` | writes target memory with `MmCopyVirtualMemory`           |
| `OP_GET_PROCESS`  | scans process list using `ZwQuerySystemInformation`       |
| `OP_GET_MODULE`   | walks target PEB loader list after `KeStackAttachProcess` |

# building

open `driver.sln` in visual studio with WDK installed.

targets:
- `km` -> x64 kernel driver
- `um` -> x64 usermode client

signed driver not needed if loading through kdmapper.

# usage

1. run DebugView as admin with kernel capture enabled
2. map driver with kdmapper

3. look in debugview for:

```
[km] request slot at 0xFFFF...
```

4. paste that address into `um/main.cpp`
5. rebuild client
6. run it

current test just grabs `notepad.exe`, resolves base address, reads MZ header.

# current state

still broken in multiple ways.

current behavior:

- driver maps fine
- no instant BSOD
- after a few minutes windows slowly dies
- keyboard shortcuts stop responding
- system hangs
- reboot sometimes triggers multiple BSODs before stabilizing again

probably caused by one or more of these:

- hook firing in bad contexts
- deadlocking inside win32k
- missing proper unload cleanup
- unsupported MDL flag abuse corrupting memory under pressure

`RemoveHook()` exists but unload isnt wired properly yet.

# known issues

- request slot address still manual
- no proper shared section between UM and KM
- current `SendRequest()` implementation is technically fucked because UM writes a userspace pointer into kernel memory directly
- `COMM_MAGIC` is public (`0xCAFEBABE`) so change it if u actually care