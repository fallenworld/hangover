/*
 * Copyright 2017 André Hentschel
 * Copyright 2018 Stefan Dösinger for CodeWeavers
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

#define COBJMACROS

#include <initguid.h>
#include <windows.h>
#include <wincodec.h>
#include <wincodecsdk.h>

#include "windows-user-services.h"
#include "dll_list.h"

#include "thunk/qemu_windows.h"
#include "thunk/qemu_wincodec.h"

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/rbtree.h>

#include "qemu_windowscodecs.h"

WINE_DEFAULT_DEBUG_CHANNEL(qemu_wic);

#include "istream_wrapper_impl.h"

struct qemu_dll_init
{
    struct qemu_syscall super;
    uint64_t reason;
    uint64_t guest_iunknown_addref;
    uint64_t guest_iunknown_release;
    uint64_t guest_bitmap_source_getsize;
    uint64_t guest_bitmap_source_getpixelformat;
    uint64_t guest_bitmap_source_copypixels;
    uint64_t guest_bitmap_source_getresolution;
    uint64_t guest_bitmap_source_copypalette;
    uint64_t guest_block_reader_getpixelformat;
    uint64_t guest_block_reader_getcount;
    uint64_t guest_block_reader_getreaderbyindex;
    uint64_t guest_reader_getmetadataformat;
    uint64_t guest_reader_getvalue;
    struct istream_wrapper_funcs istream;
};

struct guest_bitmap_source_getsize
{
    uint64_t source;
    uint64_t width, height;
};

struct guest_bitmap_source_getpixelformat
{
    uint64_t source;
    uint64_t fmt;
};

struct guest_bitmap_source_copypixels
{
    uint64_t source;
    uint64_t rect, stride, size, buffer;
};

struct guest_bitmap_source_getresolution
{
    uint64_t source;
    uint64_t x, y;
};

struct guest_bitmap_source_copypalette
{
    uint64_t source;
    uint64_t palette;
};

struct guest_block_reader_getpixelformat
{
    uint64_t reader;
    uint64_t fmt;
};

struct guest_block_reader_getcount
{
    uint64_t reader;
    uint64_t count;
};

struct guest_block_reader_getreaderbyindex
{
    uint64_t reader;
    uint64_t index;
    uint64_t out;
};

struct guest_reader_getmetadataformat
{
    uint64_t reader;
    uint64_t fmt;
};

struct guest_reader_getvalue
{
    uint64_t reader;
    uint64_t schema, id, value;
};

#ifdef QEMU_DLL_GUEST

static ULONG __fastcall guest_iunknown_addref(IUnknown *source)
{
    return IUnknown_AddRef(source);
}

static ULONG __fastcall guest_iunknown_release(IUnknown *source)
{
    return IUnknown_Release(source);
}

static ULONG __fastcall guest_bitmap_source_getsize(struct guest_bitmap_source_getsize *call)
{
    return IWICBitmapSource_GetSize((IWICBitmapSource *)(ULONG_PTR)call->source,
            (UINT *)(ULONG_PTR)call->width, (UINT *)(ULONG_PTR)call->height);
}

static ULONG __fastcall guest_bitmap_source_getpixelformat(struct guest_bitmap_source_getpixelformat *call)
{
    return IWICBitmapSource_GetPixelFormat((IWICBitmapSource *)(ULONG_PTR)call->source,
            (WICPixelFormatGUID *)(ULONG_PTR)call->fmt);
}

static ULONG __fastcall guest_bitmap_source_copypixels(struct guest_bitmap_source_copypixels *call)
{
    return IWICBitmapSource_CopyPixels((IWICBitmapSource *)(ULONG_PTR)call->source,
            (const WICRect *)(ULONG_PTR)call->rect, call->stride, call->size, (BYTE *)(ULONG_PTR)call->buffer);
}

static ULONG __fastcall guest_bitmap_source_getresolution(struct guest_bitmap_source_getresolution *call)
{
    return IWICBitmapSource_GetResolution((IWICBitmapSource *)(ULONG_PTR)call->source,
            (double *)(ULONG_PTR)call->x, (double *)(ULONG_PTR)call->y);
}

static ULONG __fastcall guest_bitmap_source_copypalette(struct guest_bitmap_source_copypalette *call)
{
    struct qemu_wic_palette *pal = (struct qemu_wic_palette *)(ULONG_PTR)call->palette;

    WICPalette_init_guest(pal);

    return IWICBitmapSource_CopyPalette((IWICBitmapSource *)(ULONG_PTR)call->source, &pal->IWICPalette_iface);
}

static ULONG __fastcall guest_block_reader_getpixelformat(struct guest_block_reader_getpixelformat *call)
{
    return IWICMetadataBlockReader_GetContainerFormat((IWICMetadataBlockReader *)(ULONG_PTR)call->reader,
            (WICPixelFormatGUID *)(ULONG_PTR)call->fmt);
}

static ULONG __fastcall guest_block_reader_getcount(struct guest_block_reader_getcount *call)
{
    return IWICMetadataBlockReader_GetCount((IWICMetadataBlockReader *)(ULONG_PTR)call->reader,
            (UINT *)(ULONG_PTR)call->count);
}

static ULONG __fastcall guest_block_reader_getreaderbyindex(struct guest_block_reader_getreaderbyindex *call)
{
    IWICMetadataBlockReader *reader = (IWICMetadataBlockReader *)(ULONG_PTR)call->reader;
    IWICMetadataReader *out;
    HRESULT hr;

    hr = IWICMetadataBlockReader_GetReaderByIndex(reader, call->index, &out);
    call->out = (ULONG_PTR)out;
    return hr;
}

static ULONG __fastcall guest_reader_getmetadataformat(struct guest_reader_getmetadataformat *call)
{
    return IWICMetadataReader_GetMetadataFormat((IWICMetadataReader *)(ULONG_PTR)call->reader,
            (GUID *)(ULONG_PTR)call->fmt);
}

static ULONG __fastcall guest_reader_getvalue(struct guest_reader_getvalue *call)
{
    return IWICMetadataReader_GetValue((IWICMetadataReader *)(ULONG_PTR)call->reader,
            (PROPVARIANT *)(ULONG_PTR)call->schema, (PROPVARIANT *)(ULONG_PTR)call->id,
            (PROPVARIANT *)(ULONG_PTR)call->value);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD reason, void *reserved)
{
    struct qemu_dll_init call;
    HMODULE dxgi;

    if (reason == DLL_PROCESS_ATTACH)
    {
        call.super.id = QEMU_SYSCALL_ID(CALL_INIT_DLL);
        call.reason = DLL_PROCESS_ATTACH;
        call.guest_iunknown_addref = (ULONG_PTR)guest_iunknown_addref;
        call.guest_iunknown_release = (ULONG_PTR)guest_iunknown_release;
        call.guest_bitmap_source_getsize = (ULONG_PTR)guest_bitmap_source_getsize;
        call.guest_bitmap_source_getpixelformat = (ULONG_PTR)guest_bitmap_source_getpixelformat;
        call.guest_bitmap_source_copypixels = (ULONG_PTR)guest_bitmap_source_copypixels;
        call.guest_bitmap_source_getresolution = (ULONG_PTR)guest_bitmap_source_getresolution;
        call.guest_bitmap_source_copypalette = (ULONG_PTR)guest_bitmap_source_copypalette;
        call.guest_block_reader_getpixelformat = (ULONG_PTR)guest_block_reader_getpixelformat;
        call.guest_block_reader_getcount = (ULONG_PTR)guest_block_reader_getcount;
        call.guest_block_reader_getreaderbyindex = (ULONG_PTR)guest_block_reader_getreaderbyindex;
        call.guest_reader_getmetadataformat = (ULONG_PTR)guest_reader_getmetadataformat;
        call.guest_reader_getvalue = (ULONG_PTR)guest_reader_getvalue;
        istream_wrapper_get_funcs(&call.istream);
        qemu_syscall(&call.super);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        call.super.id = QEMU_SYSCALL_ID(CALL_INIT_DLL);
        call.reason = DLL_PROCESS_DETACH;
        qemu_syscall(&call.super);
    }

    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    WINE_FIXME("Stub\n");
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    WINE_ERR("Should not be called on the wrapper.\n");
    return E_FAIL;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    WINE_ERR("Should not be called on the wrapper.\n");
    return E_FAIL;
}

#else

static uint64_t guest_iunknown_addref;
static uint64_t guest_iunknown_release;
static uint64_t guest_bitmap_source_getsize;
static uint64_t guest_bitmap_source_getpixelformat;
static uint64_t guest_bitmap_source_copypixels;
static uint64_t guest_bitmap_source_getresolution;
static uint64_t guest_bitmap_source_copypalette;
static uint64_t guest_block_reader_getpixelformat;
static uint64_t guest_block_reader_getcount;
static uint64_t guest_block_reader_getreaderbyindex;
static uint64_t guest_reader_getmetadataformat;
static uint64_t guest_reader_getvalue;

static void qemu_init_dll(struct qemu_syscall *call)
{
    struct qemu_dll_init *c = (struct qemu_dll_init *)call;

    switch (c->reason)
    {
        case DLL_PROCESS_ATTACH:
            guest_iunknown_addref = c->guest_iunknown_addref;
            guest_iunknown_release = c->guest_iunknown_release;
            guest_bitmap_source_getsize = c->guest_bitmap_source_getsize;
            guest_bitmap_source_getpixelformat = c->guest_bitmap_source_getpixelformat;
            guest_bitmap_source_copypixels = c->guest_bitmap_source_copypixels;
            guest_bitmap_source_getresolution = c->guest_bitmap_source_getresolution;
            guest_bitmap_source_copypalette = c->guest_bitmap_source_copypalette;
            guest_block_reader_getpixelformat = c->guest_block_reader_getpixelformat;
            guest_block_reader_getcount = c->guest_block_reader_getcount;
            guest_block_reader_getreaderbyindex = c->guest_block_reader_getreaderbyindex;
            guest_reader_getmetadataformat = c->guest_reader_getmetadataformat;
            guest_reader_getvalue = c->guest_reader_getvalue;
            istream_wrapper_set_funcs(&c->istream);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
}

const struct qemu_ops *qemu_ops;

static const syscall_handler dll_functions[] =
{
    qemu_ComponentFactory_AddRef,
    qemu_ComponentFactory_create_host,
    qemu_ComponentFactory_CreateBitmap,
    qemu_ComponentFactory_CreateBitmapClipper,
    qemu_ComponentFactory_CreateBitmapFlipRotator,
    qemu_ComponentFactory_CreateBitmapFromHBITMAP,
    qemu_ComponentFactory_CreateBitmapFromHICON,
    qemu_ComponentFactory_CreateBitmapFromMemory,
    qemu_ComponentFactory_CreateBitmapFromSource,
    qemu_ComponentFactory_CreateBitmapFromSourceRect,
    qemu_ComponentFactory_CreateBitmapScaler,
    qemu_ComponentFactory_CreateColorContext,
    qemu_ComponentFactory_CreateColorTransformer,
    qemu_ComponentFactory_CreateComponentEnumerator,
    qemu_ComponentFactory_CreateComponentInfo,
    qemu_ComponentFactory_CreateDecoder,
    qemu_ComponentFactory_CreateDecoderFromFileHandle,
    qemu_ComponentFactory_CreateDecoderFromFilename,
    qemu_ComponentFactory_CreateDecoderFromStream,
    qemu_ComponentFactory_CreateEncoder,
    qemu_ComponentFactory_CreateEncoderPropertyBag,
    qemu_ComponentFactory_CreateFastMetadataEncoderFromDecoder,
    qemu_ComponentFactory_CreateFastMetadataEncoderFromFrameDecode,
    qemu_ComponentFactory_CreateFormatConverter,
    qemu_ComponentFactory_CreateMetadataReader,
    qemu_ComponentFactory_CreateMetadataReaderFromContainer,
    qemu_ComponentFactory_CreateMetadataWriter,
    qemu_ComponentFactory_CreateMetadataWriterFromReader,
    qemu_ComponentFactory_CreatePalette,
    qemu_ComponentFactory_CreateQueryReaderFromBlockReader,
    qemu_ComponentFactory_CreateQueryWriter,
    qemu_ComponentFactory_CreateQueryWriterFromBlockWriter,
    qemu_ComponentFactory_CreateQueryWriterFromReader,
    qemu_ComponentFactory_CreateStream,
    qemu_ComponentFactory_QueryInterface,
    qemu_ComponentFactory_Release,
    qemu_EnumUnknown_AddRef,
    qemu_EnumUnknown_Clone,
    qemu_EnumUnknown_Next,
    qemu_EnumUnknown_QueryInterface,
    qemu_EnumUnknown_Release,
    qemu_EnumUnknown_Reset,
    qemu_EnumUnknown_Skip,
    qemu_IMILBitmapImpl_CopyPalette,
    qemu_IMILBitmapImpl_CopyPixels,
    qemu_IMILBitmapImpl_GetPixelFormat,
    qemu_IMILBitmapImpl_GetResolution,
    qemu_IMILBitmapImpl_GetSize,
    qemu_IMILBitmapImpl_QueryInterface,
    qemu_IMILBitmapImpl_UnknownMethod1,
    qemu_IMILUnknown1Impl_QueryInterface,
    qemu_IMILUnknown2Impl_QueryInterface,
    qemu_IMILUnknown2Impl_UnknownMethod1,
    qemu_init_dll,
    qemu_MetadataHandler_AddRef,
    qemu_MetadataHandler_create_host,
    qemu_MetadataHandler_GetClassID,
    qemu_MetadataHandler_GetCount,
    qemu_MetadataHandler_GetEnumerator,
    qemu_MetadataHandler_GetMetadataFormat,
    qemu_MetadataHandler_GetMetadataHandlerInfo,
    qemu_MetadataHandler_GetSizeMax,
    qemu_MetadataHandler_GetValue,
    qemu_MetadataHandler_GetValueByIndex,
    qemu_MetadataHandler_IsDirty,
    qemu_MetadataHandler_Load,
    qemu_MetadataHandler_LoadEx,
    qemu_MetadataHandler_QueryInterface,
    qemu_MetadataHandler_Release,
    qemu_MetadataHandler_RemoveValue,
    qemu_MetadataHandler_RemoveValueByIndex,
    qemu_MetadataHandler_Save,
    qemu_MetadataHandler_SaveEx,
    qemu_MetadataHandler_SetValue,
    qemu_MetadataHandler_SetValueByIndex,
    qemu_PropertyBag_AddRef,
    qemu_PropertyBag_CountProperties,
    qemu_PropertyBag_GetPropertyInfo,
    qemu_PropertyBag_LoadObject,
    qemu_PropertyBag_QueryInterface,
    qemu_PropertyBag_Read,
    qemu_PropertyBag_Release,
    qemu_PropertyBag_Write,
    qemu_WICBitmap_AddRef,
    qemu_WICBitmap_CopyPalette,
    qemu_WICBitmap_CopyPixels,
    qemu_WICBitmap_GetPixelFormat,
    qemu_WICBitmap_GetResolution,
    qemu_WICBitmap_GetSize,
    qemu_WICBitmap_Lock,
    qemu_WICBitmap_QueryInterface,
    qemu_WICBitmap_Release,
    qemu_WICBitmap_SetPalette,
    qemu_WICBitmap_SetResolution,
    qemu_WICBitmapClipper_AddRef,
    qemu_WICBitmapClipper_CopyPalette,
    qemu_WICBitmapClipper_CopyPixels,
    qemu_WICBitmapClipper_GetPixelFormat,
    qemu_WICBitmapClipper_GetResolution,
    qemu_WICBitmapClipper_GetSize,
    qemu_WICBitmapClipper_Initialize,
    qemu_WICBitmapClipper_QueryInterface,
    qemu_WICBitmapClipper_Release,
    qemu_WICBitmapDecoder_AddRef,
    qemu_WICBitmapDecoder_CopyPalette,
    qemu_WICBitmapDecoder_create_host,
    qemu_WICBitmapDecoder_GetColorContexts,
    qemu_WICBitmapDecoder_GetContainerFormat,
    qemu_WICBitmapDecoder_GetDecoderInfo,
    qemu_WICBitmapDecoder_GetFrame,
    qemu_WICBitmapDecoder_GetFrameCount,
    qemu_WICBitmapDecoder_GetMetadataQueryReader,
    qemu_WICBitmapDecoder_GetPreview,
    qemu_WICBitmapDecoder_GetThumbnail,
    qemu_WICBitmapDecoder_Initialize,
    qemu_WICBitmapDecoder_QueryCapability,
    qemu_WICBitmapDecoder_QueryInterface,
    qemu_WICBitmapDecoder_Release,
    qemu_WICBitmapDecoderInfo_CreateInstance,
    qemu_WICBitmapDecoderInfo_DoesSupportAnimation,
    qemu_WICBitmapDecoderInfo_DoesSupportChromaKey,
    qemu_WICBitmapDecoderInfo_DoesSupportLossless,
    qemu_WICBitmapDecoderInfo_DoesSupportMultiframe,
    qemu_WICBitmapDecoderInfo_GetColorManagementVersion,
    qemu_WICBitmapDecoderInfo_GetContainerFormat,
    qemu_WICBitmapDecoderInfo_GetDeviceManufacturer,
    qemu_WICBitmapDecoderInfo_GetDeviceModels,
    qemu_WICBitmapDecoderInfo_GetFileExtensions,
    qemu_WICBitmapDecoderInfo_GetMimeTypes,
    qemu_WICBitmapDecoderInfo_GetPatterns,
    qemu_WICBitmapDecoderInfo_GetPixelFormats,
    qemu_WICBitmapDecoderInfo_MatchesMimeType,
    qemu_WICBitmapDecoderInfo_MatchesPattern,
    qemu_WICBitmapEncoder_AddRef,
    qemu_WICBitmapEncoder_Commit,
    qemu_WICBitmapEncoder_create_host,
    qemu_WICBitmapEncoder_CreateNewFrame,
    qemu_WICBitmapEncoder_GetContainerFormat,
    qemu_WICBitmapEncoder_GetEncoderInfo,
    qemu_WICBitmapEncoder_GetMetadataQueryWriter,
    qemu_WICBitmapEncoder_Initialize,
    qemu_WICBitmapEncoder_QueryInterface,
    qemu_WICBitmapEncoder_Release,
    qemu_WICBitmapEncoder_SetColorContexts,
    qemu_WICBitmapEncoder_SetPalette,
    qemu_WICBitmapEncoder_SetPreview,
    qemu_WICBitmapEncoder_SetThumbnail,
    qemu_WICBitmapEncoderInfo_CreateInstance,
    qemu_WICBitmapEncoderInfo_DoesSupportAnimation,
    qemu_WICBitmapEncoderInfo_DoesSupportChromaKey,
    qemu_WICBitmapEncoderInfo_DoesSupportLossless,
    qemu_WICBitmapEncoderInfo_DoesSupportMultiframe,
    qemu_WICBitmapEncoderInfo_GetColorManagementVersion,
    qemu_WICBitmapEncoderInfo_GetContainerFormat,
    qemu_WICBitmapEncoderInfo_GetDeviceManufacturer,
    qemu_WICBitmapEncoderInfo_GetDeviceModels,
    qemu_WICBitmapEncoderInfo_GetFileExtensions,
    qemu_WICBitmapEncoderInfo_GetMimeTypes,
    qemu_WICBitmapEncoderInfo_GetPixelFormats,
    qemu_WICBitmapEncoderInfo_MatchesMimeType,
    qemu_WICBitmapFrameDecode_AddRef,
    qemu_WICBitmapFrameDecode_CopyPalette,
    qemu_WICBitmapFrameDecode_CopyPixels,
    qemu_WICBitmapFrameDecode_GetColorContexts,
    qemu_WICBitmapFrameDecode_GetMetadataQueryReader,
    qemu_WICBitmapFrameDecode_GetPixelFormat,
    qemu_WICBitmapFrameDecode_GetResolution,
    qemu_WICBitmapFrameDecode_GetSize,
    qemu_WICBitmapFrameDecode_GetThumbnail,
    qemu_WICBitmapFrameDecode_QueryInterface,
    qemu_WICBitmapFrameDecode_Release,
    qemu_WICBitmapFrameEncode_AddRef,
    qemu_WICBitmapFrameEncode_Commit,
    qemu_WICBitmapFrameEncode_GetMetadataQueryWriter,
    qemu_WICBitmapFrameEncode_Initialize,
    qemu_WICBitmapFrameEncode_QueryInterface,
    qemu_WICBitmapFrameEncode_Release,
    qemu_WICBitmapFrameEncode_SetColorContexts,
    qemu_WICBitmapFrameEncode_SetPalette,
    qemu_WICBitmapFrameEncode_SetPixelFormat,
    qemu_WICBitmapFrameEncode_SetResolution,
    qemu_WICBitmapFrameEncode_SetSize,
    qemu_WICBitmapFrameEncode_SetThumbnail,
    qemu_WICBitmapFrameEncode_WritePixels,
    qemu_WICBitmapFrameEncode_WriteSource,
    qemu_WICBitmapLock_AddRef,
    qemu_WICBitmapLock_GetDataPointer,
    qemu_WICBitmapLock_GetPixelFormat,
    qemu_WICBitmapLock_GetSize,
    qemu_WICBitmapLock_GetStride,
    qemu_WICBitmapLock_QueryInterface,
    qemu_WICBitmapLock_Release,
    qemu_WICColorContext_AddRef,
    qemu_WICColorContext_GetExifColorSpace,
    qemu_WICColorContext_GetProfileBytes,
    qemu_WICColorContext_GetType,
    qemu_WICColorContext_InitializeFromExifColorSpace,
    qemu_WICColorContext_InitializeFromFilename,
    qemu_WICColorContext_InitializeFromMemory,
    qemu_WICColorContext_QueryInterface,
    qemu_WICColorContext_Release,
    qemu_WICComponentInfo_AddRef,
    qemu_WICComponentInfo_GetAuthor,
    qemu_WICComponentInfo_GetCLSID,
    qemu_WICComponentInfo_GetComponentType,
    qemu_WICComponentInfo_GetFriendlyName,
    qemu_WICComponentInfo_GetSigningStatus,
    qemu_WICComponentInfo_GetSpecVersion,
    qemu_WICComponentInfo_GetVendorGUID,
    qemu_WICComponentInfo_GetVersion,
    qemu_WICComponentInfo_QueryInterface,
    qemu_WICComponentInfo_Release,
    qemu_WICConvertBitmapSource,
    qemu_WICCreateBitmapFromSectionEx,
    qemu_WICDecoder_MetadataBlockReader_GetContainerFormat,
    qemu_WICDecoder_MetadataBlockReader_GetCount,
    qemu_WICDecoder_MetadataBlockReader_GetEnumerator,
    qemu_WICDecoder_MetadataBlockReader_GetReaderByIndex,
    qemu_WICEnumMetadataItem_AddRef,
    qemu_WICEnumMetadataItem_Clone,
    qemu_WICEnumMetadataItem_Next,
    qemu_WICEnumMetadataItem_QueryInterface,
    qemu_WICEnumMetadataItem_Release,
    qemu_WICEnumMetadataItem_Reset,
    qemu_WICEnumMetadataItem_Skip,
    qemu_WICFormatConverter_AddRef,
    qemu_WICFormatConverter_CanConvert,
    qemu_WICFormatConverter_CopyPalette,
    qemu_WICFormatConverter_CopyPixels,
    qemu_WICFormatConverter_create_host,
    qemu_WICFormatConverter_GetPixelFormat,
    qemu_WICFormatConverter_GetResolution,
    qemu_WICFormatConverter_GetSize,
    qemu_WICFormatConverter_Initialize,
    qemu_WICFormatConverter_QueryInterface,
    qemu_WICFormatConverter_Release,
    qemu_WICFormatConverterInfo_CreateInstance,
    qemu_WICFormatConverterInfo_GetPixelFormats,
    qemu_WICMapGuidToShortName,
    qemu_WICMapSchemaToName,
    qemu_WICMapShortNameToGuid,
    qemu_WICMetadataBlockReader_GetContainerFormat,
    qemu_WICMetadataBlockReader_GetCount,
    qemu_WICMetadataBlockReader_GetEnumerator,
    qemu_WICMetadataBlockReader_GetReaderByIndex,
    qemu_WICMetadataQueryReader_AddRef,
    qemu_WICMetadataQueryReader_GetContainerFormat,
    qemu_WICMetadataQueryReader_GetEnumerator,
    qemu_WICMetadataQueryReader_GetLocation,
    qemu_WICMetadataQueryReader_GetMetadataByName,
    qemu_WICMetadataQueryReader_QueryInterface,
    qemu_WICMetadataQueryReader_Release,
    qemu_WICMetadataReaderInfo_CreateInstance,
    qemu_WICMetadataReaderInfo_DoesRequireFixedSize,
    qemu_WICMetadataReaderInfo_DoesRequireFullStream,
    qemu_WICMetadataReaderInfo_DoesSupportPadding,
    qemu_WICMetadataReaderInfo_GetContainerFormats,
    qemu_WICMetadataReaderInfo_GetDeviceManufacturer,
    qemu_WICMetadataReaderInfo_GetDeviceModels,
    qemu_WICMetadataReaderInfo_GetMetadataFormat,
    qemu_WICMetadataReaderInfo_GetPatterns,
    qemu_WICMetadataReaderInfo_MatchesPattern,
    qemu_WICPalette_AddRef,
    qemu_WICPalette_GetColorCount,
    qemu_WICPalette_GetColors,
    qemu_WICPalette_GetType,
    qemu_WICPalette_HasAlpha,
    qemu_WICPalette_InitializeCustom,
    qemu_WICPalette_InitializeFromBitmap,
    qemu_WICPalette_InitializeFromPalette,
    qemu_WICPalette_InitializePredefined,
    qemu_WICPalette_IsBlackWhite,
    qemu_WICPalette_IsGrayscale,
    qemu_WICPalette_QueryInterface,
    qemu_WICPalette_Release,
    qemu_WICPixelFormatInfo2_GetBitsPerPixel,
    qemu_WICPixelFormatInfo2_GetChannelCount,
    qemu_WICPixelFormatInfo2_GetChannelMask,
    qemu_WICPixelFormatInfo2_GetColorContext,
    qemu_WICPixelFormatInfo2_GetFormatGUID,
    qemu_WICPixelFormatInfo2_GetNumericRepresentation,
    qemu_WICPixelFormatInfo2_SupportsTransparency,
    qemu_WICStream_AddRef,
    qemu_WICStream_Clone,
    qemu_WICStream_Commit,
    qemu_WICStream_CopyTo,
    qemu_WICStream_InitializeFromFilename,
    qemu_WICStream_InitializeFromIStream,
    qemu_WICStream_InitializeFromIStreamRegion,
    qemu_WICStream_InitializeFromMemory,
    qemu_WICStream_LockRegion,
    qemu_WICStream_QueryInterface,
    qemu_WICStream_Read,
    qemu_WICStream_Release,
    qemu_WICStream_Revert,
    qemu_WICStream_Seek,
    qemu_WICStream_SetSize,
    qemu_WICStream_Stat,
    qemu_WICStream_UnlockRegion,
    qemu_WICStream_Write,
};

struct wine_rb_tree info_tree;

static int info_tree_compare(const void *key, const struct wine_rb_entry *entry)
{
    struct qemu_wic_info *data = WINE_RB_ENTRY_VALUE(entry, struct qemu_wic_info, entry);

    if ((ULONG_PTR)key > (ULONG_PTR)data->host)
        return 1;
    else if ((ULONG_PTR)key == (ULONG_PTR)data->host)
        return 0;
    else
        return -1;
}

const WINAPI syscall_handler *qemu_dll_register(const struct qemu_ops *ops, uint32_t *dll_num)
{
    WINE_TRACE("Loading host-side windowscodecs wrapper.\n");

    qemu_ops = ops;
    *dll_num = QEMU_CURRENT_DLL;

    wine_rb_init(&info_tree, info_tree_compare);

    return dll_functions;
}

static inline struct qemu_bitmap_source *impl_from_IWICBitmapSource(IWICBitmapSource *iface)
{
    return CONTAINING_RECORD(iface, struct qemu_bitmap_source, IWICBitmapSource_iface);
}

static HRESULT WINAPI qemu_bitmap_source_QueryInterface(IWICBitmapSource *iface, REFIID iid, void **ppv)
{
    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid))
    {
        *ppv = iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI qemu_bitmap_source_AddRef(IWICBitmapSource *iface)
{
    struct qemu_bitmap_source *source = impl_from_IWICBitmapSource(iface);
    ULONG ref = InterlockedIncrement(&source->ref);

    WINE_TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI qemu_bitmap_source_Release(IWICBitmapSource *iface)
{
    struct qemu_bitmap_source *source = impl_from_IWICBitmapSource(iface);
    ULONG ref = InterlockedDecrement(&source->ref), ref2;

    WINE_TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        WINE_TRACE("Calling guest release method.\n");
        ref2 = qemu_ops->qemu_execute(QEMU_G2H(guest_iunknown_release), source->guest);
        WINE_TRACE("Guest release method returned %u.\n", ref2);
        HeapFree(GetProcessHeap(), 0, source);
    }

    return ref;
}

static HRESULT WINAPI qemu_bitmap_source_GetSize(IWICBitmapSource *iface, UINT *width, UINT *height)
{
    struct qemu_bitmap_source *source = impl_from_IWICBitmapSource(iface);
    struct guest_bitmap_source_getsize call;
    HRESULT hr;

    WINE_TRACE("\n");
    call.source = source->guest;
    call.width = QEMU_H2G(width);
    call.height = QEMU_H2G(height);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_bitmap_source_getsize), QEMU_H2G(&call));

    return hr;
}

static HRESULT WINAPI qemu_bitmap_source_GetPixelFormat(IWICBitmapSource *iface,
        WICPixelFormatGUID *format)
{
    struct qemu_bitmap_source *source = impl_from_IWICBitmapSource(iface);
    struct guest_bitmap_source_getpixelformat call;
    HRESULT hr;

    WINE_TRACE("\n");
    call.source = source->guest;
    call.fmt = QEMU_H2G(format);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_bitmap_source_getpixelformat), QEMU_H2G(&call));

    return hr;
}

static HRESULT WINAPI qemu_bitmap_source_GetResolution(IWICBitmapSource *iface,
        double *dpiX, double *dpiY)
{
    struct qemu_bitmap_source *source = impl_from_IWICBitmapSource(iface);
    struct guest_bitmap_source_getresolution call;
    HRESULT hr;

    WINE_TRACE("\n");
    call.source = source->guest;
    call.x = QEMU_H2G(dpiX);
    call.y = QEMU_H2G(dpiY);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_bitmap_source_getresolution), QEMU_H2G(&call));

    return hr;
}

static HRESULT WINAPI qemu_bitmap_source_CopyPalette(IWICBitmapSource *iface,
        IWICPalette *palette)
{
    struct qemu_bitmap_source *source = impl_from_IWICBitmapSource(iface);
    struct qemu_wic_palette temp_palette = {0};
    struct guest_bitmap_source_copypalette call;
    HRESULT hr;

    WINE_FIXME("The stack alloc is begging for trouble.\n");
    temp_palette.host = palette;
    call.source = source->guest;
    call.palette = QEMU_H2G(&temp_palette);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_bitmap_source_copypalette), QEMU_H2G(&call));

    return hr;
}

static HRESULT WINAPI qemu_bitmap_source_CopyPixels(IWICBitmapSource *iface,
        const WICRect *rc, UINT stride, UINT buffer_size, BYTE *buffer)
{
    struct qemu_bitmap_source *source = impl_from_IWICBitmapSource(iface);
    struct guest_bitmap_source_copypixels call;
    HRESULT hr;

    WINE_TRACE("\n");
    call.source = source->guest;
    call.rect = QEMU_H2G(rc);
    call.stride = stride;
    call.size = buffer_size;
    call.buffer = QEMU_H2G(buffer);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_bitmap_source_copypixels), QEMU_H2G(&call));

    return hr;
}

static const IWICBitmapSourceVtbl qemu_bitmap_source_vtbl =
{
    qemu_bitmap_source_QueryInterface,
    qemu_bitmap_source_AddRef,
    qemu_bitmap_source_Release,
    qemu_bitmap_source_GetSize,
    qemu_bitmap_source_GetPixelFormat,
    qemu_bitmap_source_GetResolution,
    qemu_bitmap_source_CopyPalette,
    qemu_bitmap_source_CopyPixels
};

struct qemu_bitmap_source *bitmap_source_wrapper_create(uint64_t guest)
{
    struct qemu_bitmap_source *ret;
    ULONG ref;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    ret->IWICBitmapSource_iface.lpVtbl = &qemu_bitmap_source_vtbl;
    ret->guest = guest;
    ret->ref = 1;

    WINE_TRACE("Calling guest addref method.\n");
    ref = qemu_ops->qemu_execute(QEMU_G2H(guest_iunknown_addref), guest);
    WINE_TRACE("Guest addref returned %u.\n", ref);

    return ret;
}

static inline struct qemu_mdr *impl_from_IWICMetadataReader(IWICMetadataReader *iface)
{
    return CONTAINING_RECORD(iface, struct qemu_mdr, IWICMetadataReader_iface);
}

static HRESULT WINAPI mdr_QueryInterface(IWICMetadataReader *iface, REFIID iid, void **out)
{
    WINE_TRACE("%p,%s,%p\n", iface, wine_dbgstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IWICMetadataReader))
    {
        *out = iface;
        IWICMetadataReader_AddRef(iface);
        return S_OK;
    }

    WINE_WARN("unknown iid %s\n", wine_dbgstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mdr_AddRef(IWICMetadataReader *iface)
{
    struct qemu_mdr *mdr = impl_from_IWICMetadataReader(iface);
    ULONG ref = InterlockedIncrement(&mdr->ref);

    WINE_TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI mdr_Release(IWICMetadataReader *iface)
{
    struct qemu_mdr *mdr = impl_from_IWICMetadataReader(iface);
    ULONG ref = InterlockedDecrement(&mdr->ref), ref2;

    WINE_TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        WINE_TRACE("Calling guest release method.\n");
        ref2 = qemu_ops->qemu_execute(QEMU_G2H(guest_iunknown_release), mdr->guest);
        WINE_TRACE("Guest release method returned %u.\n", ref2);
        HeapFree(GetProcessHeap(), 0, mdr);
    }

    return ref;
}

static HRESULT WINAPI mdr_GetMetadataFormat(IWICMetadataReader *iface, GUID *format)
{
    struct qemu_mdr *reader = impl_from_IWICMetadataReader(iface);
    struct guest_reader_getmetadataformat call;
    HRESULT hr;

    WINE_TRACE("\n");
    call.reader = reader->guest;
    call.fmt = QEMU_H2G(format);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_reader_getmetadataformat), QEMU_H2G(&call));

    return hr;
}

static HRESULT WINAPI mdr_GetMetadataHandlerInfo(IWICMetadataReader *iface, IWICMetadataHandlerInfo **handler)
{
    WINE_FIXME("Stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI mdr_GetCount(IWICMetadataReader *iface, UINT *count)
{
    WINE_FIXME("Stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI mdr_GetValueByIndex(IWICMetadataReader *iface, UINT index, PROPVARIANT *schema,
        PROPVARIANT *id, PROPVARIANT *value)
{
    WINE_FIXME("Stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI mdr_GetValue(IWICMetadataReader *iface, const PROPVARIANT *schema, const PROPVARIANT *id,
        PROPVARIANT *value)
{
    struct qemu_mdr *reader = impl_from_IWICMetadataReader(iface);
    struct guest_reader_getvalue call;
    HRESULT hr;
    struct qemu_PROPVARIANT schema32, id32, value32;
    WCHAR *bounce_str = NULL, *bounce_str2 = NULL, *str;
    size_t len;

    WINE_TRACE("\n");
    call.reader = reader->guest;
#if GUEST_BIT == HOST_BIT
    call.schema = QEMU_H2G(schema);
    call.id = QEMU_H2G(id);
    call.value = QEMU_H2G(value);
#else
    if (schema)
    {
        PROPVARIANT_h2g(&schema32, schema);

        /* The host lib passes pointers to static const WCHAR[] data. This here is ugly, the problem
         * might as well happen with non-string values. */
        switch(schema->vt)
        {
            case VT_LPWSTR:
                str = schema->pwszVal;
                if ((ULONG_PTR)str > ~0U)
                {
                    len = (lstrlenW(str) + 1) * sizeof(*str);
                    bounce_str = HeapAlloc(GetProcessHeap(), 0, len);
                    memcpy(bounce_str, str, len);
                    schema32.u1.pwszVal = QEMU_H2G(bounce_str);
                }
        }

        call.schema = QEMU_H2G(&schema32);
    }
    else
    {
        call.schema = 0;
    }

    if (id)
    {
        PROPVARIANT_h2g(&id32, id);

        switch(id->vt)
        {
            case VT_LPWSTR:
                str = id->pwszVal;
                if ((ULONG_PTR)str > ~0U)
                {
                    len = (lstrlenW(str) + 1) * sizeof(*str);
                    bounce_str2 = HeapAlloc(GetProcessHeap(), 0, len);
                    memcpy(bounce_str2, str, len);
                    id32.u1.pwszVal = QEMU_H2G(bounce_str2);
                }
        }

        call.id = QEMU_H2G(&id32);
    }
    else
    {
        call.id = 0;
    }

    if (value)
    {
        PROPVARIANT_h2g(&value32, value);
        call.value = QEMU_H2G(&value32);
    }
    else
    {
        call.value = 0;
    }
#endif

    WINE_TRACE("Calling guest function %p.\n", QEMU_G2H(guest_reader_getvalue));
    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_reader_getvalue), QEMU_H2G(&call));
    WINE_TRACE("Guest function returned %#x.\n", hr);

#if GUEST_BIT != HOST_BIT
    if (value)
        PROPVARIANT_g2h(value, &value32);

    HeapFree(GetProcessHeap(), 0, bounce_str);
    HeapFree(GetProcessHeap(), 0, bounce_str2);
#endif

    if (value && value->vt == VT_UNKNOWN)
    {
        if ((ULONG_PTR)value->punkVal == reader->guest)
            value->punkVal = (IUnknown *)&reader->IWICMetadataReader_iface;
        else
            WINE_FIXME("Unexpected interface %p returned.\n", value->punkVal);

        IWICMetadataReader_AddRef(&reader->IWICMetadataReader_iface);
        qemu_ops->qemu_execute(QEMU_G2H(guest_iunknown_release), reader->guest);
    }

    return hr;
}

static HRESULT WINAPI mdr_GetEnumerator(IWICMetadataReader *iface, IWICEnumMetadataItem **enumerator)
{
    WINE_FIXME("Stub!\n");
    return E_NOTIMPL;
}

static const IWICMetadataReaderVtbl mdr_wrapper_vtbl =
{
    mdr_QueryInterface,
    mdr_AddRef,
    mdr_Release,
    mdr_GetMetadataFormat,
    mdr_GetMetadataHandlerInfo,
    mdr_GetCount,
    mdr_GetValueByIndex,
    mdr_GetValue,
    mdr_GetEnumerator
};

static HRESULT WINAPI mdbr_QueryInterface(IWICMetadataBlockReader *iface, REFIID iid, void **out)
{
    WINE_FIXME("Stub!\n");
    return E_NOINTERFACE;
}

static inline struct qemu_mdbr *impl_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, struct qemu_mdbr, IWICMetadataBlockReader_iface);
}

static ULONG WINAPI mdbr_AddRef(IWICMetadataBlockReader *iface)
{
    struct qemu_mdbr *mdbr = impl_from_IWICMetadataBlockReader(iface);
    ULONG ref = InterlockedIncrement(&mdbr->ref);

    WINE_TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI mdbr_Release(IWICMetadataBlockReader *iface)
{
    struct qemu_mdbr *mdbr = impl_from_IWICMetadataBlockReader(iface);
    ULONG ref = InterlockedDecrement(&mdbr->ref), ref2;

    WINE_TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        WINE_TRACE("Calling guest release method.\n");
        ref2 = qemu_ops->qemu_execute(QEMU_G2H(guest_iunknown_release), mdbr->guest);
        WINE_TRACE("Guest release method returned %u.\n", ref2);
        HeapFree(GetProcessHeap(), 0, mdbr);
    }

    return ref;
}

static HRESULT WINAPI mdbr_GetContainerFormat(IWICMetadataBlockReader *iface, GUID *format)
{
    struct qemu_mdbr *reader = impl_from_IWICMetadataBlockReader(iface);
    struct guest_block_reader_getpixelformat call;
    HRESULT hr;

    WINE_TRACE("\n");
    call.reader = reader->guest;
    call.fmt = QEMU_H2G(format);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_block_reader_getpixelformat), QEMU_H2G(&call));

    return hr;
}

static HRESULT WINAPI mdbr_GetCount(IWICMetadataBlockReader *iface, UINT *count)
{
    struct qemu_mdbr *reader = impl_from_IWICMetadataBlockReader(iface);
    struct guest_block_reader_getcount call;
    HRESULT hr;

    WINE_TRACE("\n");
    call.reader = reader->guest;
    call.count = QEMU_H2G(count);

    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_block_reader_getcount), QEMU_H2G(&call));

    return hr;
}

static HRESULT WINAPI mdbr_GetReaderByIndex(IWICMetadataBlockReader *iface, UINT index, IWICMetadataReader **out)
{
    struct qemu_mdbr *reader = impl_from_IWICMetadataBlockReader(iface);
    struct guest_block_reader_getreaderbyindex call;
    HRESULT hr;
    struct qemu_mdr *mdr;

    WINE_TRACE("\n");
    *out = NULL;

    call.reader = reader->guest;
    call.index = index;

    mdr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*mdr));
    if (!mdr)
    {
        WINE_WARN("Out of memory.\n");
        return E_OUTOFMEMORY;
    }

    WINE_TRACE("Calling guest function %p.\n", QEMU_G2H(guest_block_reader_getreaderbyindex));
    hr = qemu_ops->qemu_execute(QEMU_G2H(guest_block_reader_getreaderbyindex), QEMU_H2G(&call));
    WINE_TRACE("Guest function returned %#x.\n", hr);

    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, mdr);
        return hr;
    }

    mdr->ref = 1;
    mdr->guest = call.out;
    mdr->IWICMetadataReader_iface.lpVtbl = &mdr_wrapper_vtbl;

    *out = &mdr->IWICMetadataReader_iface;

    return hr;
}

static HRESULT WINAPI mdbr_GetEnumerator(IWICMetadataBlockReader *iface, IEnumUnknown **enumerator)
{
    WINE_FIXME("Stub!\n");
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl mdbr_wrapper_vtbl =
{
    mdbr_QueryInterface,
    mdbr_AddRef,
    mdbr_Release,
    mdbr_GetContainerFormat,
    mdbr_GetCount,
    mdbr_GetReaderByIndex,
    mdbr_GetEnumerator
};

struct qemu_mdbr *mdbr_wrapper_create(uint64_t guest)
{
    struct qemu_mdbr *ret;
    ULONG ref;

    if (!guest)
        return NULL;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    ret->IWICMetadataBlockReader_iface.lpVtbl = &mdbr_wrapper_vtbl;
    ret->guest = guest;
    ret->ref = 1;

    WINE_TRACE("Calling guest addref method.\n");
    ref = qemu_ops->qemu_execute(QEMU_G2H(guest_iunknown_addref), guest);
    WINE_TRACE("Guest addref returned %u.\n", ref);

    return ret;
}

#endif
