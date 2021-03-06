#ifndef HAVE_QEMU_THUNK_D3D11_H
#define HAVE_QEMU_THUNK_D3D11_H

#include "thunk/qemu_dxgi.h"

struct qemu_D3D11_SUBRESOURCE_DATA
{
    qemu_ptr pSysMem;
    UINT SysMemPitch;
    UINT SysMemSlicePitch;
};

static inline void D3D11_SUBRESOURCE_DATA_g2h(D3D11_SUBRESOURCE_DATA *host,
            const struct qemu_D3D11_SUBRESOURCE_DATA *guest)
{
    host->pSysMem = (void *)(ULONG_PTR)guest->pSysMem;
    host->SysMemPitch = guest->SysMemPitch;
    host->SysMemSlicePitch = guest->SysMemSlicePitch;
}

struct qemu_D3D11_INPUT_ELEMENT_DESC
{
    qemu_ptr SemanticName;
    UINT SemanticIndex;
    DXGI_FORMAT Format;
    UINT InputSlot;
    UINT AlignedByteOffset;
    D3D11_INPUT_CLASSIFICATION InputSlotClass;
    UINT InstanceDataStepRate;
};

static inline void D3D11_INPUT_ELEMENT_DESC_g2h(D3D11_INPUT_ELEMENT_DESC *host,
        const struct qemu_D3D11_INPUT_ELEMENT_DESC *guest)
{
    host->SemanticName = (const char *)(ULONG_PTR)guest->SemanticName;
    host->SemanticIndex = guest->SemanticIndex;
    host->Format = guest->Format;
    host->InputSlot = guest->InputSlot;
    host->AlignedByteOffset = guest->AlignedByteOffset;
    host->InputSlotClass = guest->InputSlotClass;
    host->InstanceDataStepRate = guest->InstanceDataStepRate;
}

struct qemu_D3D11_MAPPED_SUBRESOURCE
{
    qemu_ptr pData;
    UINT RowPitch;
    UINT DepthPitch;
};

static inline void D3D11_MAPPED_SUBRESOURCE_h2g(struct qemu_D3D11_MAPPED_SUBRESOURCE *guest,
        const D3D11_MAPPED_SUBRESOURCE *host)
{
    guest->pData = (ULONG_PTR)host->pData;
    guest->RowPitch = host->RowPitch;
    guest->DepthPitch = host->DepthPitch;
}

struct qemu_D3D11_SO_DECLARATION_ENTRY
{
    UINT Stream;
    qemu_ptr SemanticName;
    UINT SemanticIndex;
    BYTE StartComponent;
    BYTE ComponentCount;
    BYTE OutputSlot;
};

static inline void D3D11_SO_DECLARATION_ENTRY_g2h(D3D11_SO_DECLARATION_ENTRY *host,
        const struct qemu_D3D11_SO_DECLARATION_ENTRY *guest)
{
    host->Stream = guest->Stream;
    host->SemanticName = (const char *)(ULONG_PTR)guest->SemanticName;
    host->SemanticIndex = guest->SemanticIndex;
    host->StartComponent = guest->StartComponent;
    host->ComponentCount = guest->ComponentCount;
    host->OutputSlot = guest->OutputSlot;
}

struct qemu_D3D10_MAPPED_TEXTURE2D
{
    qemu_ptr pData;
    UINT RowPitch;
};

static inline void D3D10_MAPPED_TEXTURE2D_h2g(struct qemu_D3D10_MAPPED_TEXTURE2D *guest,
        const D3D10_MAPPED_TEXTURE2D *host)
{
    guest->pData = (ULONG_PTR)host->pData;
    guest->RowPitch = host->RowPitch;
}

struct qemu_D3D10_MAPPED_TEXTURE3D
{
    qemu_ptr pData;
    UINT RowPitch;
    UINT DepthPitch;
};

static inline void D3D10_MAPPED_TEXTURE3D_h2g(struct qemu_D3D10_MAPPED_TEXTURE3D *guest,
        const D3D10_MAPPED_TEXTURE3D *host)
{
    guest->pData = (ULONG_PTR)host->pData;
    guest->RowPitch = host->RowPitch;
    guest->DepthPitch = host->DepthPitch;
}

#endif
