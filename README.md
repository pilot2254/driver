> [!CAUTION]
> doesnt work

# my kernel driver

im NOT responsible for what u do with this. u fuck up ur pc or get banned thats on u.<br />
built while learning windows kernel and low level c

## what it is

simple kernel driver that reads and writes memory of any process.<br />
my own learning project. and definitely not beginner friendly. if u need hand holding dont touch this.

## does

- read process memory
- write process memory
- get process by name
- ring 0 access
- works with [kdmapper](https://github.com/TheCruZ/kdmapper)

## needs

- windows 10 or 11
- visual studio 2022
- wdk
- kdmapper
- vm. dont be dumb and run this on ur main box

## build

1. open driver.sln
2. release x64  (release is recommended)
3. build
4. driver.sys ends up in x64 release

## usage in vm

1. move driver.sys and kdmapper.exe to vm
2. run as admin
```
kdmapper.exe driver.sys //or you can just simply drag the driver into kdmapper
```
3. talk to it using the client app

## layout

```
driver/
├── driver.c          main entry + ioctl
├── driver.h          structs & defs
├── memory.c          read write logic
└── communication.c   future stuff
```

## ioctls

- `IOCTL_READ_MEMORY`
- `IOCTL_WRITE_MEMORY`
- `IOCTL_GET_PROCESS`

names say it all

## notes

- buggy drivers = bsod
- test in vm only
- misuse can flag anticheats
- educational. nothing more

## contact

discord: @michal.flaska

## Support me
- **BTC:** `bc1qr8x3k39mn7sz2l9kyk8j8293xf5spm2wsxymh9`
