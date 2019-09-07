/*
    Making NTSTATUS available is slightly awkward. See:
    https://kirkshoop.github.io/2011/09/20/ntstatus.html
*/

/* Win32 user-mode API */
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winternl.h>
#include <setupapi.h>
#include <unknwn.h>
#include <windns.h>
#include <ws2tcpip.h>
#include <dinput.h>
#include <xinput.h>
#include <d3d9.h>

/* Win32 kernel-mode definitions */
#ifdef __GNUC__
/* MinGW needs to include this for PHYSICAL_ADDRESS to be defined.
   The MS SDK throws a bunch of duplicate symbol errors instead. */
#include <ntdef.h>
#else
#include <winnt.h>
#endif
#include <devioctl.h>
#include <ntdddisk.h>
#include <ntddser.h>
#include <ntstatus.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
