# win32-darkmode
Example application shows how to use undocumented dark mode API introduced in Windows 10 1809.

## Delayload branch
The [delayload](https://github.com/ysc3839/win32-darkmode/tree/delayload) branch uses self generated import libraries (using the same way as [dumplib](https://github.com/Mattiwatti/dumplib)) and then link to delay load table. So that we don't need to write a lot of `GetProcAddress` manually.
