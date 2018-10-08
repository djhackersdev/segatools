/* Win32 user-mode API */
#include <windows.h>
#include <winternl.h>
#include <setupapi.h>
#include <unknwn.h>
#include <d3d9.h>

/* Win32 kernel-mode definitions */
#include <ntdef.h>
#include <ntstatus.h>
#include <devioctl.h>
#include <ntddser.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
