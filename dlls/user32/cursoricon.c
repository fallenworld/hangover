/*
 * Copyright 2017 André Hentschel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* NOTE: The guest side uses mingw's headers. The host side uses Wine's headers. */

#include <windows.h>
#include <stdio.h>

#include "thunk/qemu_windows.h"

#include "windows-user-services.h"
#include "dll_list.h"
#include "qemu_user32.h"

#ifndef QEMU_DLL_GUEST
#include <wine/debug.h>
#include <wine/unicode.h>
WINE_DEFAULT_DEBUG_CHANNEL(qemu_user32);
#endif


struct qemu_CreateIconFromResourceEx
{
    struct qemu_syscall super;
    uint64_t bits;
    uint64_t cbSize;
    uint64_t bIcon;
    uint64_t dwVersion;
    uint64_t width;
    uint64_t height;
    uint64_t cFlag;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HICON WINAPI CreateIconFromResourceEx(LPBYTE bits, DWORD cbSize, WINBOOL bIcon, DWORD dwVersion, int width, int height, UINT cFlag)
{
    struct qemu_CreateIconFromResourceEx call;
    call.super.id = QEMU_SYSCALL_ID(CALL_CREATEICONFROMRESOURCEEX);
    call.bits = (ULONG_PTR)bits;
    call.cbSize = (ULONG_PTR)cbSize;
    call.bIcon = (ULONG_PTR)bIcon;
    call.dwVersion = (ULONG_PTR)dwVersion;
    call.width = (ULONG_PTR)width;
    call.height = (ULONG_PTR)height;
    call.cFlag = (ULONG_PTR)cFlag;

    qemu_syscall(&call.super);

    return (HICON)(ULONG_PTR)call.super.iret;
}

#else

void qemu_CreateIconFromResourceEx(struct qemu_syscall *call)
{
    struct qemu_CreateIconFromResourceEx *c = (struct qemu_CreateIconFromResourceEx *)call;
    WINE_FIXME("Unverified!\n");
    c->super.iret = (ULONG_PTR)CreateIconFromResourceEx(QEMU_G2H(c->bits), c->cbSize, c->bIcon, c->dwVersion, c->width, c->height, c->cFlag);
}

#endif

struct qemu_CreateIconFromResource
{
    struct qemu_syscall super;
    uint64_t bits;
    uint64_t cbSize;
    uint64_t bIcon;
    uint64_t dwVersion;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HICON WINAPI CreateIconFromResource(LPBYTE bits, DWORD cbSize, WINBOOL bIcon, DWORD dwVersion)
{
    struct qemu_CreateIconFromResource call;
    call.super.id = QEMU_SYSCALL_ID(CALL_CREATEICONFROMRESOURCE);
    call.bits = (ULONG_PTR)bits;
    call.cbSize = (ULONG_PTR)cbSize;
    call.bIcon = (ULONG_PTR)bIcon;
    call.dwVersion = (ULONG_PTR)dwVersion;

    qemu_syscall(&call.super);

    return (HICON)(ULONG_PTR)call.super.iret;
}

#else

void qemu_CreateIconFromResource(struct qemu_syscall *call)
{
    struct qemu_CreateIconFromResource *c = (struct qemu_CreateIconFromResource *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)CreateIconFromResource(QEMU_G2H(c->bits), c->cbSize, c->bIcon, c->dwVersion);
}

#endif

struct qemu_CreateCursor
{
    struct qemu_syscall super;
    uint64_t hInstance;
    uint64_t xHotSpot;
    uint64_t yHotSpot;
    uint64_t nWidth;
    uint64_t nHeight;
    uint64_t lpANDbits;
    uint64_t lpXORbits;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI CreateCursor(HINSTANCE hInstance, INT xHotSpot, INT yHotSpot, INT nWidth, INT nHeight, LPCVOID lpANDbits, LPCVOID lpXORbits)
{
    struct qemu_CreateCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_CREATECURSOR);
    call.hInstance = (ULONG_PTR)hInstance;
    call.xHotSpot = (ULONG_PTR)xHotSpot;
    call.yHotSpot = (ULONG_PTR)yHotSpot;
    call.nWidth = (ULONG_PTR)nWidth;
    call.nHeight = (ULONG_PTR)nHeight;
    call.lpANDbits = (ULONG_PTR)lpANDbits;
    call.lpXORbits = (ULONG_PTR)lpXORbits;

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_CreateCursor(struct qemu_syscall *call)
{
    struct qemu_CreateCursor *c = (struct qemu_CreateCursor *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)CreateCursor(QEMU_G2H(c->hInstance), c->xHotSpot, c->yHotSpot, c->nWidth, c->nHeight, QEMU_G2H(c->lpANDbits), QEMU_G2H(c->lpXORbits));
}

#endif

struct qemu_CreateIcon
{
    struct qemu_syscall super;
    uint64_t hInstance;
    uint64_t nWidth;
    uint64_t nHeight;
    uint64_t bPlanes;
    uint64_t bBitsPixel;
    uint64_t lpANDbits;
    uint64_t lpXORbits;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HICON WINAPI CreateIcon(HINSTANCE hInstance, int nWidth, int nHeight, BYTE bPlanes, BYTE bBitsPixel, const BYTE *lpANDbits, const BYTE *lpXORbits)
{
    struct qemu_CreateIcon call;
    call.super.id = QEMU_SYSCALL_ID(CALL_CREATEICON);
    call.hInstance = (ULONG_PTR)hInstance;
    call.nWidth = (ULONG_PTR)nWidth;
    call.nHeight = (ULONG_PTR)nHeight;
    call.bPlanes = (ULONG_PTR)bPlanes;
    call.bBitsPixel = (ULONG_PTR)bBitsPixel;
    call.lpANDbits = (ULONG_PTR)lpANDbits;
    call.lpXORbits = (ULONG_PTR)lpXORbits;

    qemu_syscall(&call.super);

    return (HICON)(ULONG_PTR)call.super.iret;
}

#else

void qemu_CreateIcon(struct qemu_syscall *call)
{
    struct qemu_CreateIcon *c = (struct qemu_CreateIcon *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)CreateIcon(QEMU_G2H(c->hInstance), c->nWidth, c->nHeight, c->bPlanes, c->bBitsPixel, QEMU_G2H(c->lpANDbits), QEMU_G2H(c->lpXORbits));
}

#endif

struct qemu_CopyIcon
{
    struct qemu_syscall super;
    uint64_t hIcon;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HICON WINAPI CopyIcon(HICON hIcon)
{
    struct qemu_CopyIcon call;
    call.super.id = QEMU_SYSCALL_ID(CALL_COPYICON);
    call.hIcon = (ULONG_PTR)hIcon;

    qemu_syscall(&call.super);

    return (HICON)(ULONG_PTR)call.super.iret;
}

#else

void qemu_CopyIcon(struct qemu_syscall *call)
{
    struct qemu_CopyIcon *c = (struct qemu_CopyIcon *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)CopyIcon(QEMU_G2H(c->hIcon));
}

#endif

struct qemu_DestroyIcon
{
    struct qemu_syscall super;
    uint64_t hIcon;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI DestroyIcon(HICON hIcon)
{
    struct qemu_DestroyIcon call;
    call.super.id = QEMU_SYSCALL_ID(CALL_DESTROYICON);
    call.hIcon = (ULONG_PTR)hIcon;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_DestroyIcon(struct qemu_syscall *call)
{
    struct qemu_DestroyIcon *c = (struct qemu_DestroyIcon *)call;
    WINE_TRACE("\n");
    c->super.iret = DestroyIcon(QEMU_G2H(c->hIcon));
}

#endif

struct qemu_DestroyCursor
{
    struct qemu_syscall super;
    uint64_t hCursor;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI DestroyCursor(HCURSOR hCursor)
{
    struct qemu_DestroyCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_DESTROYCURSOR);
    call.hCursor = (ULONG_PTR)hCursor;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_DestroyCursor(struct qemu_syscall *call)
{
    struct qemu_DestroyCursor *c = (struct qemu_DestroyCursor *)call;
    WINE_TRACE("\n");
    c->super.iret = DestroyCursor(QEMU_G2H(c->hCursor));
}

#endif

struct qemu_DrawIcon
{
    struct qemu_syscall super;
    uint64_t hdc;
    uint64_t x;
    uint64_t y;
    uint64_t hIcon;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI DrawIcon(HDC hdc, INT x, INT y, HICON hIcon)
{
    struct qemu_DrawIcon call;
    call.super.id = QEMU_SYSCALL_ID(CALL_DRAWICON);
    call.hdc = (ULONG_PTR)hdc;
    call.x = (ULONG_PTR)x;
    call.y = (ULONG_PTR)y;
    call.hIcon = (ULONG_PTR)hIcon;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_DrawIcon(struct qemu_syscall *call)
{
    struct qemu_DrawIcon *c = (struct qemu_DrawIcon *)call;
    WINE_TRACE("\n");
    c->super.iret = DrawIcon(QEMU_G2H(c->hdc), c->x, c->y, QEMU_G2H(c->hIcon));
}

#endif

struct qemu_SetCursor
{
    struct qemu_syscall super;
    uint64_t hCursor;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI SetCursor(HCURSOR hCursor)
{
    struct qemu_SetCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_SETCURSOR);
    call.hCursor = (ULONG_PTR)hCursor;

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_SetCursor(struct qemu_syscall *call)
{
    struct qemu_SetCursor *c = (struct qemu_SetCursor *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)SetCursor(QEMU_G2H(c->hCursor));
}

#endif

struct qemu_ShowCursor
{
    struct qemu_syscall super;
    uint64_t bShow;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI INT WINAPI ShowCursor(BOOL bShow)
{
    struct qemu_ShowCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_SHOWCURSOR);
    call.bShow = (ULONG_PTR)bShow;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_ShowCursor(struct qemu_syscall *call)
{
    struct qemu_ShowCursor *c = (struct qemu_ShowCursor *)call;
    WINE_TRACE("\n");
    c->super.iret = ShowCursor(c->bShow);
}

#endif

struct qemu_GetCursor
{
    struct qemu_syscall super;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI GetCursor(void)
{
    struct qemu_GetCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_GETCURSOR);

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_GetCursor(struct qemu_syscall *call)
{
    struct qemu_GetCursor *c = (struct qemu_GetCursor *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)GetCursor();
}

#endif

struct qemu_ClipCursor
{
    struct qemu_syscall super;
    uint64_t rect;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI ClipCursor(const RECT *rect)
{
    struct qemu_ClipCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_CLIPCURSOR);
    call.rect = (ULONG_PTR)rect;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_ClipCursor(struct qemu_syscall *call)
{
    struct qemu_ClipCursor *c = (struct qemu_ClipCursor *)call;
    WINE_TRACE("\n");
    c->super.iret = ClipCursor(QEMU_G2H(c->rect));
}

#endif

struct qemu_GetClipCursor
{
    struct qemu_syscall super;
    uint64_t rect;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI GetClipCursor(RECT *rect)
{
    struct qemu_GetClipCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_GETCLIPCURSOR);
    call.rect = (ULONG_PTR)rect;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_GetClipCursor(struct qemu_syscall *call)
{
    struct qemu_GetClipCursor *c = (struct qemu_GetClipCursor *)call;
    WINE_TRACE("\n");
    c->super.iret = GetClipCursor(QEMU_G2H(c->rect));
}

#endif

struct qemu_SetSystemCursor
{
    struct qemu_syscall super;
    uint64_t hcur;
    uint64_t id;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI SetSystemCursor(HCURSOR hcur, DWORD id)
{
    struct qemu_SetSystemCursor call;
    call.super.id = QEMU_SYSCALL_ID(CALL_SETSYSTEMCURSOR);
    call.hcur = (ULONG_PTR)hcur;
    call.id = (ULONG_PTR)id;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_SetSystemCursor(struct qemu_syscall *call)
{
    struct qemu_SetSystemCursor *c = (struct qemu_SetSystemCursor *)call;
    WINE_FIXME("Unverified!\n");
    c->super.iret = SetSystemCursor(QEMU_G2H(c->hcur), c->id);
}

#endif

struct qemu_LookupIconIdFromDirectoryEx
{
    struct qemu_syscall super;
    uint64_t xdir;
    uint64_t bIcon;
    uint64_t width;
    uint64_t height;
    uint64_t cFlag;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI INT WINAPI LookupIconIdFromDirectoryEx(LPBYTE xdir, BOOL bIcon, INT width, INT height, UINT cFlag)
{
    struct qemu_LookupIconIdFromDirectoryEx call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOOKUPICONIDFROMDIRECTORYEX);
    call.xdir = (ULONG_PTR)xdir;
    call.bIcon = (ULONG_PTR)bIcon;
    call.width = (ULONG_PTR)width;
    call.height = (ULONG_PTR)height;
    call.cFlag = (ULONG_PTR)cFlag;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_LookupIconIdFromDirectoryEx(struct qemu_syscall *call)
{
    struct qemu_LookupIconIdFromDirectoryEx *c = (struct qemu_LookupIconIdFromDirectoryEx *)call;
    WINE_FIXME("Unverified!\n");
    c->super.iret = LookupIconIdFromDirectoryEx(QEMU_G2H(c->xdir), c->bIcon, c->width, c->height, c->cFlag);
}

#endif

struct qemu_LookupIconIdFromDirectory
{
    struct qemu_syscall super;
    uint64_t dir;
    uint64_t bIcon;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI INT WINAPI LookupIconIdFromDirectory(LPBYTE dir, BOOL bIcon)
{
    struct qemu_LookupIconIdFromDirectory call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOOKUPICONIDFROMDIRECTORY);
    call.dir = (ULONG_PTR)dir;
    call.bIcon = (ULONG_PTR)bIcon;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_LookupIconIdFromDirectory(struct qemu_syscall *call)
{
    struct qemu_LookupIconIdFromDirectory *c = (struct qemu_LookupIconIdFromDirectory *)call;
    WINE_FIXME("Unverified!\n");
    c->super.iret = LookupIconIdFromDirectory(QEMU_G2H(c->dir), c->bIcon);
}

#endif

struct qemu_LoadCursorW
{
    struct qemu_syscall super;
    uint64_t hInstance;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI LoadCursorW(HINSTANCE hInstance, LPCWSTR name)
{
    struct qemu_LoadCursorW call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADCURSORW);
    call.hInstance = (ULONG_PTR)hInstance;
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadCursorW(struct qemu_syscall *call)
{
    struct qemu_LoadCursorW *c = (struct qemu_LoadCursorW *)call;
    WINE_TRACE("\n");
    /* Do not replace c->hInstance here. A NULL instance means loading the
     * cursor from user32, not the current .exe. */
    c->super.iret = (ULONG_PTR)LoadCursorW((HINSTANCE)c->hInstance, QEMU_G2H(c->name));
}

#endif

struct qemu_LoadCursorA
{
    struct qemu_syscall super;
    uint64_t hInstance;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR name)
{
    struct qemu_LoadCursorA call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADCURSORA);
    call.hInstance = (ULONG_PTR)hInstance;
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadCursorA(struct qemu_syscall *call)
{
    struct qemu_LoadCursorA *c = (struct qemu_LoadCursorA *)call;
    WINE_TRACE("\n");
    /* Do not replace c->hInstance here. A NULL instance means loading the
     * cursor from user32, not the current .exe. */
    c->super.iret = (ULONG_PTR)LoadCursorA((HINSTANCE)c->hInstance, QEMU_G2H(c->name));
}

#endif

struct qemu_LoadCursorFromFileW
{
    struct qemu_syscall super;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI LoadCursorFromFileW (LPCWSTR name)
{
    struct qemu_LoadCursorFromFileW call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADCURSORFROMFILEW);
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadCursorFromFileW(struct qemu_syscall *call)
{
    struct qemu_LoadCursorFromFileW *c = (struct qemu_LoadCursorFromFileW *)call;
    WINE_FIXME("Unverified!\n");
    c->super.iret = (ULONG_PTR)LoadCursorFromFileW(QEMU_G2H(c->name));
}

#endif

struct qemu_LoadCursorFromFileA
{
    struct qemu_syscall super;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI LoadCursorFromFileA (LPCSTR name)
{
    struct qemu_LoadCursorFromFileA call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADCURSORFROMFILEA);
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadCursorFromFileA(struct qemu_syscall *call)
{
    struct qemu_LoadCursorFromFileA *c = (struct qemu_LoadCursorFromFileA *)call;
    WINE_FIXME("Unverified!\n");
    c->super.iret = (ULONG_PTR)LoadCursorFromFileA(QEMU_G2H(c->name));
}

#endif

struct qemu_LoadIconW
{
    struct qemu_syscall super;
    uint64_t hInstance;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HICON WINAPI LoadIconW(HINSTANCE hInstance, LPCWSTR name)
{
    struct qemu_LoadIconW call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADICONW);
    call.hInstance = (ULONG_PTR)hInstance;
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HICON)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadIconW(struct qemu_syscall *call)
{
    struct qemu_LoadIconW *c = (struct qemu_LoadIconW *)call;
    WINE_TRACE("\n");

    /* Do not replace hInstance, a NULL instance means user32.dll. */
    c->super.iret = (ULONG_PTR)LoadIconW((HINSTANCE)c->hInstance, QEMU_G2H(c->name));
}

#endif

struct qemu_LoadIconA
{
    struct qemu_syscall super;
    uint64_t hInstance;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR name)
{
    struct qemu_LoadIconA call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADICONA);
    call.hInstance = (ULONG_PTR)hInstance;
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HICON)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadIconA(struct qemu_syscall *call)
{
    struct qemu_LoadIconA *c = (struct qemu_LoadIconA *)call;
    WINE_TRACE("\n");

    /* Do not replace hInstance, a NULL instance means user32.dll. */
    c->super.iret = (ULONG_PTR)LoadIconA((HINSTANCE)c->hInstance, QEMU_G2H(c->name));
}

#endif

struct qemu_GetCursorFrameInfo
{
    struct qemu_syscall super;
    uint64_t hCursor;
    uint64_t reserved;
    uint64_t istep;
    uint64_t rate_jiffies;
    uint64_t num_steps;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HCURSOR WINAPI GetCursorFrameInfo(HCURSOR hCursor, DWORD reserved, DWORD istep, DWORD *rate_jiffies, DWORD *num_steps)
{
    struct qemu_GetCursorFrameInfo call;
    call.super.id = QEMU_SYSCALL_ID(CALL_GETCURSORFRAMEINFO);
    call.hCursor = (ULONG_PTR)hCursor;
    call.reserved = (ULONG_PTR)reserved;
    call.istep = (ULONG_PTR)istep;
    call.rate_jiffies = (ULONG_PTR)rate_jiffies;
    call.num_steps = (ULONG_PTR)num_steps;

    qemu_syscall(&call.super);

    return (HCURSOR)(ULONG_PTR)call.super.iret;
}

#else

void qemu_GetCursorFrameInfo(struct qemu_syscall *call)
{
    struct qemu_GetCursorFrameInfo *c = (struct qemu_GetCursorFrameInfo *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)GetCursorFrameInfo(QEMU_G2H(c->hCursor), c->reserved, c->istep, QEMU_G2H(c->rate_jiffies), QEMU_G2H(c->num_steps));
}

#endif

struct qemu_GetIconInfo
{
    struct qemu_syscall super;
    uint64_t hIcon;
    uint64_t iconinfo;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI GetIconInfo(HICON hIcon, PICONINFO iconinfo)
{
    struct qemu_GetIconInfo call;
    call.super.id = QEMU_SYSCALL_ID(CALL_GETICONINFO);
    call.hIcon = (ULONG_PTR)hIcon;
    call.iconinfo = (ULONG_PTR)iconinfo;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_GetIconInfo(struct qemu_syscall *call)
{
    struct qemu_GetIconInfo *c = (struct qemu_GetIconInfo *)call;
    ICONINFO stack, *info = &stack;
    struct qemu_ICONINFO *info32;
    WINE_TRACE("\n");

#if GUEST_BIT == HOST_BIT
    info = QEMU_G2H(c->iconinfo);
#else
    info32 = QEMU_G2H(c->iconinfo);
    if (!info32)
        info = NULL;
#endif

    c->super.iret = GetIconInfo(QEMU_G2H(c->hIcon), info);

#if GUEST_BIT != HOST_BIT
    if (c->super.iret)
        ICONINFO_h2g(info32, info);
#endif
}

#endif

struct qemu_GetIconInfoExA
{
    struct qemu_syscall super;
    uint64_t icon;
    uint64_t info;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI GetIconInfoExA(HICON icon, ICONINFOEXA *info)
{
    struct qemu_GetIconInfoExA call;
    call.super.id = QEMU_SYSCALL_ID(CALL_GETICONINFOEXA);
    call.icon = (ULONG_PTR)icon;
    call.info = (ULONG_PTR)info;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_GetIconInfoExA(struct qemu_syscall *call)
{
    struct qemu_GetIconInfoExA *c = (struct qemu_GetIconInfoExA *)call;
    ICONINFOEXA stack, *info = &stack;
    struct qemu_ICONINFOEXA *info32;

    WINE_TRACE("\n");
#if GUEST_BIT == HOST_BIT
    info = QEMU_G2H(c->info);
#else
    info32 = QEMU_G2H(c->info);
    if (info32)
    {
        if (info32->cbSize == sizeof(*info32))
        {
            info->cbSize = sizeof(*info);
        }
        else
        {
            info->cbSize = 0;
        }
    }
    else
    {
        info = NULL;
    }
#endif

    c->super.iret = GetIconInfoExA(QEMU_G2H(c->icon), info);

#if GUEST_BIT != HOST_BIT
    if (c->super.iret)
        ICONINFOEXA_h2g(info32, info);
#endif
}

#endif

struct qemu_GetIconInfoExW
{
    struct qemu_syscall super;
    uint64_t icon;
    uint64_t info;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI GetIconInfoExW(HICON icon, ICONINFOEXW *info)
{
    struct qemu_GetIconInfoExW call;
    call.super.id = QEMU_SYSCALL_ID(CALL_GETICONINFOEXW);
    call.icon = (ULONG_PTR)icon;
    call.info = (ULONG_PTR)info;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_GetIconInfoExW(struct qemu_syscall *call)
{
    struct qemu_GetIconInfoExW *c = (struct qemu_GetIconInfoExW *)call;
    ICONINFOEXW stack, *info = &stack;
    struct qemu_ICONINFOEXW *info32;

    WINE_TRACE("\n");
#if GUEST_BIT == HOST_BIT
    info = QEMU_G2H(c->info);
#else
    info32 = QEMU_G2H(c->info);
    if (info32)
    {
        if (info32->cbSize == sizeof(*info32))
        {
            info->cbSize = sizeof(*info);
        }
        else
        {
            info->cbSize = 0;
        }
    }
    else
    {
        info = NULL;
    }
#endif

    c->super.iret = GetIconInfoExW(QEMU_G2H(c->icon), info);

#if GUEST_BIT != HOST_BIT
    if (c->super.iret)
        ICONINFOEXW_h2g(info32, info);
#endif
}

#endif

struct qemu_CreateIconIndirect
{
    struct qemu_syscall super;
    uint64_t iconinfo;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HICON WINAPI CreateIconIndirect(PICONINFO iconinfo)
{
    struct qemu_CreateIconIndirect call;
    call.super.id = QEMU_SYSCALL_ID(CALL_CREATEICONINDIRECT);
    call.iconinfo = (ULONG_PTR)iconinfo;

    qemu_syscall(&call.super);

    return (HICON)(ULONG_PTR)call.super.iret;
}

#else

void qemu_CreateIconIndirect(struct qemu_syscall *call)
{
    struct qemu_CreateIconIndirect *c = (struct qemu_CreateIconIndirect *)call;
    ICONINFO stack, *info = &stack;
    struct qemu_ICONINFO *info32;
    WINE_TRACE("\n");

#if GUEST_BIT == HOST_BIT
    info = QEMU_G2H(c->iconinfo);
#else
    info32 = QEMU_G2H(c->iconinfo);
    if (info32)
        ICONINFO_g2h(info, info32);
    else
        info = NULL;
#endif

    c->super.iret = (ULONG_PTR)CreateIconIndirect(info);
}

#endif

struct qemu_DrawIconEx
{
    struct qemu_syscall super;
    uint64_t hdc;
    uint64_t x0;
    uint64_t y0;
    uint64_t hIcon;
    uint64_t cxWidth;
    uint64_t cyWidth;
    uint64_t istep;
    uint64_t hbr;
    uint64_t flags;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI BOOL WINAPI DrawIconEx(HDC hdc, INT x0, INT y0, HICON hIcon, INT cxWidth, INT cyWidth, UINT istep, HBRUSH hbr, UINT flags)
{
    struct qemu_DrawIconEx call;
    call.super.id = QEMU_SYSCALL_ID(CALL_DRAWICONEX);
    call.hdc = (ULONG_PTR)hdc;
    call.x0 = (ULONG_PTR)x0;
    call.y0 = (ULONG_PTR)y0;
    call.hIcon = (ULONG_PTR)hIcon;
    call.cxWidth = (ULONG_PTR)cxWidth;
    call.cyWidth = (ULONG_PTR)cyWidth;
    call.istep = (ULONG_PTR)istep;
    call.hbr = (ULONG_PTR)hbr;
    call.flags = (ULONG_PTR)flags;

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_DrawIconEx(struct qemu_syscall *call)
{
    struct qemu_DrawIconEx *c = (struct qemu_DrawIconEx *)call;
    WINE_TRACE("\n");
    c->super.iret = DrawIconEx(QEMU_G2H(c->hdc), c->x0, c->y0, QEMU_G2H(c->hIcon), c->cxWidth, c->cyWidth, c->istep, QEMU_G2H(c->hbr), c->flags);
}

#endif

struct qemu_LoadImageA
{
    struct qemu_syscall super;
    uint64_t hinst;
    uint64_t name;
    uint64_t type;
    uint64_t desiredx;
    uint64_t desiredy;
    uint64_t loadflags;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HANDLE WINAPI LoadImageA(HINSTANCE hinst, LPCSTR name, UINT type, INT desiredx, INT desiredy, UINT loadflags)
{
    struct qemu_LoadImageA call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADIMAGEA);
    call.hinst = (ULONG_PTR)hinst;
    call.name = (ULONG_PTR)name;
    call.type = (ULONG_PTR)type;
    call.desiredx = (ULONG_PTR)desiredx;
    call.desiredy = (ULONG_PTR)desiredy;
    call.loadflags = (ULONG_PTR)loadflags;

    qemu_syscall(&call.super);

    return (HANDLE)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadImageA(struct qemu_syscall *call)
{
    struct qemu_LoadImageA *c = (struct qemu_LoadImageA *)call;
    WINE_TRACE("\n");

    /* Do not replace hInstance, a NULL instance means user32.dll. */
    c->super.iret = (ULONG_PTR)LoadImageA((HINSTANCE)c->hinst, QEMU_G2H(c->name), c->type, c->desiredx,
            c->desiredy, c->loadflags);
}

#endif

struct qemu_LoadImageW
{
    struct qemu_syscall super;
    uint64_t hinst;
    uint64_t name;
    uint64_t type;
    uint64_t desiredx;
    uint64_t desiredy;
    uint64_t loadflags;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HANDLE WINAPI LoadImageW(HINSTANCE hinst, LPCWSTR name, UINT type, INT desiredx, INT desiredy, UINT loadflags)
{
    struct qemu_LoadImageW call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADIMAGEW);
    call.hinst = (ULONG_PTR)hinst;
    call.name = (ULONG_PTR)name;
    call.type = (ULONG_PTR)type;
    call.desiredx = (ULONG_PTR)desiredx;
    call.desiredy = (ULONG_PTR)desiredy;
    call.loadflags = (ULONG_PTR)loadflags;

    qemu_syscall(&call.super);

    return (HANDLE)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadImageW(struct qemu_syscall *call)
{
    struct qemu_LoadImageW *c = (struct qemu_LoadImageW *)call;
    WINE_TRACE("\n");

    /* Do not replace hInstance, a NULL instance means user32.dll. */
    c->super.iret = (ULONG_PTR)LoadImageW((HINSTANCE)c->hinst, QEMU_G2H(c->name), c->type, c->desiredx,
            c->desiredy, c->loadflags);
}

#endif

struct qemu_CopyImage
{
    struct qemu_syscall super;
    uint64_t hnd;
    uint64_t type;
    uint64_t desiredx;
    uint64_t desiredy;
    uint64_t flags;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HANDLE WINAPI CopyImage(HANDLE hnd, UINT type, INT desiredx, INT desiredy, UINT flags)
{
    struct qemu_CopyImage call;
    call.super.id = QEMU_SYSCALL_ID(CALL_COPYIMAGE);
    call.hnd = (ULONG_PTR)hnd;
    call.type = (ULONG_PTR)type;
    call.desiredx = (ULONG_PTR)desiredx;
    call.desiredy = (ULONG_PTR)desiredy;
    call.flags = (ULONG_PTR)flags;

    qemu_syscall(&call.super);

    return (HANDLE)(ULONG_PTR)call.super.iret;
}

#else

void qemu_CopyImage(struct qemu_syscall *call)
{
    struct qemu_CopyImage *c = (struct qemu_CopyImage *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)CopyImage(QEMU_G2H(c->hnd), c->type, c->desiredx, c->desiredy, c->flags);
}

#endif

struct qemu_LoadBitmapW
{
    struct qemu_syscall super;
    uint64_t instance;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HBITMAP WINAPI LoadBitmapW(HINSTANCE instance, LPCWSTR name)
{
    struct qemu_LoadBitmapW call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADBITMAPW);
    call.instance = (ULONG_PTR)instance;
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HBITMAP)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadBitmapW(struct qemu_syscall *call)
{
    struct qemu_LoadBitmapW *c = (struct qemu_LoadBitmapW *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)LoadBitmapW(QEMU_G2H(c->instance), QEMU_G2H(c->name));
}

#endif

struct qemu_LoadBitmapA
{
    struct qemu_syscall super;
    uint64_t instance;
    uint64_t name;
};

#ifdef QEMU_DLL_GUEST

WINUSERAPI HBITMAP WINAPI LoadBitmapA(HINSTANCE instance, LPCSTR name)
{
    struct qemu_LoadBitmapA call;
    call.super.id = QEMU_SYSCALL_ID(CALL_LOADBITMAPA);
    call.instance = (ULONG_PTR)instance;
    call.name = (ULONG_PTR)name;

    qemu_syscall(&call.super);

    return (HBITMAP)(ULONG_PTR)call.super.iret;
}

#else

void qemu_LoadBitmapA(struct qemu_syscall *call)
{
    struct qemu_LoadBitmapA *c = (struct qemu_LoadBitmapA *)call;
    WINE_TRACE("\n");
    c->super.iret = (ULONG_PTR)LoadBitmapA(QEMU_G2H(c->instance), QEMU_G2H(c->name));
}

#endif

