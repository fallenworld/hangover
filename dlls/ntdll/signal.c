/*
 * Copyright 2017 Stefan Dösinger for CodeWeavers
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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>
#include <winternl.h>
#include <ntdef.h>

#include "windows-user-services.h"
#include "dll_list.h"
#include "qemu_ntdll.h"

struct qemu_ExceptDebug
{
    struct qemu_syscall super;
    uint64_t string;
    uint64_t num_params;
    uint64_t p1, p2, p3, p4, p5;
};

#ifdef QEMU_DLL_GUEST

#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )

#else

#include <wine/debug.h>
#include <winternl.h>
WINE_DEFAULT_DEBUG_CHANNEL(qemu_ntdll);

#endif

#ifdef QEMU_DLL_GUEST

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10
#define EH_TARGET_UNWIND    0x20
#define EH_COLLIDED_UNWIND  0x40

typedef struct _SCOPE_TABLE
{
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;

typedef void (WINAPI *TERMINATION_HANDLER)( ULONG flags, ULONG64 frame );
typedef LONG (WINAPI *PC_LANGUAGE_EXCEPTION_HANDLER)( EXCEPTION_POINTERS *ptrs, ULONG64 frame );

#ifdef _WIN64

#define __ASM_DEFINE_FUNC(name,suffix,code) asm(".text\n\t.align 4\n\t.globl " #name suffix "\n\t.def " #name suffix "; .scl 2; .type 32; .endef\n" #name suffix ":\n\t.cfi_startproc\n\t" code "\n\t.cfi_endproc")
#define __ASM_STDCALL(args)
#define __ASM_STDCALL_FUNC(name,args,code) __ASM_DEFINE_FUNC(name,__ASM_STDCALL(args),code)

#define __ASM_NAME(name) name
#define __ASM_GLOBAL_FUNC(name,code) __ASM_DEFINE_FUNC(name,"",code)
#define __ASM_CFI(str) str

extern VOID NTAPI ntdll_RtlCaptureContext(PCONTEXT ContextRecord);

__ASM_GLOBAL_FUNC( ntdll_RtlCaptureContext,
                   "pushfq\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "movl $0x001000f,0x30(%rcx)\n\t" /* context->ContextFlags */
                   "stmxcsr 0x34(%rcx)\n\t"         /* context->MxCsr */
                   "movw %cs,0x38(%rcx)\n\t"        /* context->SegCs */
                   "movw %ds,0x3a(%rcx)\n\t"        /* context->SegDs */
                   "movw %es,0x3c(%rcx)\n\t"        /* context->SegEs */
                   "movw %fs,0x3e(%rcx)\n\t"        /* context->SegFs */
                   "movw %gs,0x40(%rcx)\n\t"        /* context->SegGs */
                   "movw %ss,0x42(%rcx)\n\t"        /* context->SegSs */
                   "popq 0x44(%rcx)\n\t"            /* context->Eflags */
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   "movq %rax,0x78(%rcx)\n\t"       /* context->Rax */
                   "movq %rcx,0x80(%rcx)\n\t"       /* context->Rcx */
                   "movq %rdx,0x88(%rcx)\n\t"       /* context->Rdx */
                   "movq %rbx,0x90(%rcx)\n\t"       /* context->Rbx */
                   "leaq 8(%rsp),%rax\n\t"
                   "movq %rax,0x98(%rcx)\n\t"       /* context->Rsp */
                   "movq %rbp,0xa0(%rcx)\n\t"       /* context->Rbp */
                   "movq %rsi,0xa8(%rcx)\n\t"       /* context->Rsi */
                   "movq %rdi,0xb0(%rcx)\n\t"       /* context->Rdi */
                   "movq %r8,0xb8(%rcx)\n\t"        /* context->R8 */
                   "movq %r9,0xc0(%rcx)\n\t"        /* context->R9 */
                   "movq %r10,0xc8(%rcx)\n\t"       /* context->R10 */
                   "movq %r11,0xd0(%rcx)\n\t"       /* context->R11 */
                   "movq %r12,0xd8(%rcx)\n\t"       /* context->R12 */
                   "movq %r13,0xe0(%rcx)\n\t"       /* context->R13 */
                   "movq %r14,0xe8(%rcx)\n\t"       /* context->R14 */
                   "movq %r15,0xf0(%rcx)\n\t"       /* context->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0xf8(%rcx)\n\t"       /* context->Rip */
                   "fxsave 0x100(%rcx)\n\t"         /* context->FtlSave */
                   "movdqa %xmm0,0x1a0(%rcx)\n\t"   /* context->Xmm0 */
                   "movdqa %xmm1,0x1b0(%rcx)\n\t"   /* context->Xmm1 */
                   "movdqa %xmm2,0x1c0(%rcx)\n\t"   /* context->Xmm2 */
                   "movdqa %xmm3,0x1d0(%rcx)\n\t"   /* context->Xmm3 */
                   "movdqa %xmm4,0x1e0(%rcx)\n\t"   /* context->Xmm4 */
                   "movdqa %xmm5,0x1f0(%rcx)\n\t"   /* context->Xmm5 */
                   "movdqa %xmm6,0x200(%rcx)\n\t"   /* context->Xmm6 */
                   "movdqa %xmm7,0x210(%rcx)\n\t"   /* context->Xmm7 */
                   "movdqa %xmm8,0x220(%rcx)\n\t"   /* context->Xmm8 */
                   "movdqa %xmm9,0x230(%rcx)\n\t"   /* context->Xmm9 */
                   "movdqa %xmm10,0x240(%rcx)\n\t"  /* context->Xmm10 */
                   "movdqa %xmm11,0x250(%rcx)\n\t"  /* context->Xmm11 */
                   "movdqa %xmm12,0x260(%rcx)\n\t"  /* context->Xmm12 */
                   "movdqa %xmm13,0x270(%rcx)\n\t"  /* context->Xmm13 */
                   "movdqa %xmm14,0x280(%rcx)\n\t"  /* context->Xmm14 */
                   "movdqa %xmm15,0x290(%rcx)\n\t"  /* context->Xmm15 */
                   "ret" );

static DWORD call_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    /* Hmm. I guess I should wrap this in some try / catch, but tricky inside the exception handler */

    /*TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
         dispatch->LanguageHandler, rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );*/
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    /*TRACE( "handler %p returned %x\n", dispatch->LanguageHandler, res );*/


    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        /* raise_status( STATUS_INVALID_DISPOSITION, rec ); */
        break;
    }

    return res;
}

/* FIXME: Those shouldn't be hardcoded here. */
static const size_t teb_size = 0x2000;
static size_t signal_stack_size;

/***********************************************************************
 *           get_signal_stack
 *
 * Get the base of the signal stack for the current thread.
 */
static inline void *get_signal_stack(void)
{
    return (char *)NtCurrentTeb() + teb_size;
}

/***********************************************************************
 *           is_inside_signal_stack
 *
 * Check if pointer is inside the signal stack.
 */
static inline BOOL is_inside_signal_stack( void *ptr )
{
    return ((char *)ptr >= (char *)get_signal_stack() &&
            (char *)ptr < (char *)get_signal_stack() + signal_stack_size);
}

/* FIXME: The Wine function I copypasted follows the Unix calling convention, with the first
 * parameter in %rdi. This is Win64 code, so the first parameter is in %rcx. I added a simple
 * mov, but it would be nicer to re-write this thing to properly source %rcx, but be careful
 * with the line that sets %rcx. */
extern void set_full_cpu_context( const CONTEXT *context );
__ASM_GLOBAL_FUNC( set_full_cpu_context,
                   "movq %rcx, %rdi\n\t"
                   "subq $40,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 40\n\t")
                   "ldmxcsr 0x34(%rdi)\n\t"         /* context->MxCsr */
                   "movw 0x38(%rdi),%ax\n\t"        /* context->SegCs */
                   "movq %rax,8(%rsp)\n\t"
                   "movw 0x42(%rdi),%ax\n\t"        /* context->SegSs */
                   "movq %rax,32(%rsp)\n\t"
                   "movq 0x44(%rdi),%rax\n\t"       /* context->Eflags */
                   "movq %rax,16(%rsp)\n\t"
                   "movq 0x80(%rdi),%rcx\n\t"       /* context->Rcx */
                   "movq 0x88(%rdi),%rdx\n\t"       /* context->Rdx */
                   "movq 0x90(%rdi),%rbx\n\t"       /* context->Rbx */
                   "movq 0x98(%rdi),%rax\n\t"       /* context->Rsp */
                   "movq %rax,24(%rsp)\n\t"
                   "movq 0xa0(%rdi),%rbp\n\t"       /* context->Rbp */
                   "movq 0xa8(%rdi),%rsi\n\t"       /* context->Rsi */
                   "movq 0xb8(%rdi),%r8\n\t"        /* context->R8 */
                   "movq 0xc0(%rdi),%r9\n\t"        /* context->R9 */
                   "movq 0xc8(%rdi),%r10\n\t"       /* context->R10 */
                   "movq 0xd0(%rdi),%r11\n\t"       /* context->R11 */
                   "movq 0xd8(%rdi),%r12\n\t"       /* context->R12 */
                   "movq 0xe0(%rdi),%r13\n\t"       /* context->R13 */
                   "movq 0xe8(%rdi),%r14\n\t"       /* context->R14 */
                   "movq 0xf0(%rdi),%r15\n\t"       /* context->R15 */
                   "movq 0xf8(%rdi),%rax\n\t"       /* context->Rip */
                   "movq %rax,(%rsp)\n\t"
                   "fxrstor 0x100(%rdi)\n\t"        /* context->FtlSave */
                   "movdqa 0x1a0(%rdi),%xmm0\n\t"   /* context->Xmm0 */
                   "movdqa 0x1b0(%rdi),%xmm1\n\t"   /* context->Xmm1 */
                   "movdqa 0x1c0(%rdi),%xmm2\n\t"   /* context->Xmm2 */
                   "movdqa 0x1d0(%rdi),%xmm3\n\t"   /* context->Xmm3 */
                   "movdqa 0x1e0(%rdi),%xmm4\n\t"   /* context->Xmm4 */
                   "movdqa 0x1f0(%rdi),%xmm5\n\t"   /* context->Xmm5 */
                   "movdqa 0x200(%rdi),%xmm6\n\t"   /* context->Xmm6 */
                   "movdqa 0x210(%rdi),%xmm7\n\t"   /* context->Xmm7 */
                   "movdqa 0x220(%rdi),%xmm8\n\t"   /* context->Xmm8 */
                   "movdqa 0x230(%rdi),%xmm9\n\t"   /* context->Xmm9 */
                   "movdqa 0x240(%rdi),%xmm10\n\t"  /* context->Xmm10 */
                   "movdqa 0x250(%rdi),%xmm11\n\t"  /* context->Xmm11 */
                   "movdqa 0x260(%rdi),%xmm12\n\t"  /* context->Xmm12 */
                   "movdqa 0x270(%rdi),%xmm13\n\t"  /* context->Xmm13 */
                   "movdqa 0x280(%rdi),%xmm14\n\t"  /* context->Xmm14 */
                   "movdqa 0x290(%rdi),%xmm15\n\t"  /* context->Xmm15 */
                   "movq 0x78(%rdi),%rax\n\t"       /* context->Rax */
                   "movq 0xb0(%rdi),%rdi\n\t"       /* context->Rdi */
                   "iretq" );


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context. Used by NtSetContextThread.
 */
static void set_cpu_context( const CONTEXT *context )
{
    DWORD flags = context->ContextFlags & ~CONTEXT_AMD64;

    /*if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        amd64_thread_data()->dr0 = context->Dr0;
        amd64_thread_data()->dr1 = context->Dr1;
        amd64_thread_data()->dr2 = context->Dr2;
        amd64_thread_data()->dr3 = context->Dr3;
        amd64_thread_data()->dr6 = context->Dr6;
        amd64_thread_data()->dr7 = context->Dr7;
    }*/
    if (flags & CONTEXT_FULL)
    {
        if (!(flags & CONTEXT_CONTROL))
        {
            //FIXME( "setting partial context (%x) not supported\n", flags );
        }
        else
            set_full_cpu_context( context );
    }
}

/**********************************************************************
 *           call_consolidate_callback
 *
 * Wrapper function to call a consolidate callback from a fake frame.
 * If the callback executes RtlUnwindEx (like for example done in C++ handlers),
 * we have to skip all frames which were already processed. To do that we
 * trick the unwinding functions into thinking the call came from somewhere
 * else. All CFI instructions are either DW_CFA_def_cfa_expression or
 * DW_CFA_expression, and the expressions have the following format:
 *
 * DW_OP_breg6; sleb128 0x10            | Load %rbp + 0x10
 * DW_OP_deref                          | Get *(%rbp + 0x10) == context
 * DW_OP_plus_uconst; uleb128 <OFFSET>  | Add offset to get struct member
 * [DW_OP_deref]                        | Dereference, only for CFA
 */
extern void * WINAPI call_consolidate_callback( CONTEXT *context,
                                                void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                EXCEPTION_RECORD *rec );
__ASM_GLOBAL_FUNC( call_consolidate_callback,
                   "pushq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "subq $0x20,%rsp\n\t"
                   "movq %rcx,0x10(%rbp)\n\t"
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x07,0x76,0x10,0x06,0x23,0x98,0x01,0x06\n\t") /* CFA    */
                   __ASM_CFI(".cfi_escape 0x10,0x03,0x06,0x76,0x10,0x06,0x23,0x90,0x01\n\t") /* %rbx   */
                   __ASM_CFI(".cfi_escape 0x10,0x04,0x06,0x76,0x10,0x06,0x23,0xa8,0x01\n\t") /* %rsi   */
                   __ASM_CFI(".cfi_escape 0x10,0x05,0x06,0x76,0x10,0x06,0x23,0xb0,0x01\n\t") /* %rdi   */
                   __ASM_CFI(".cfi_escape 0x10,0x06,0x06,0x76,0x10,0x06,0x23,0xa0,0x01\n\t") /* %rbp   */
                   __ASM_CFI(".cfi_escape 0x10,0x0c,0x06,0x76,0x10,0x06,0x23,0xd8,0x01\n\t") /* %r12   */
                   __ASM_CFI(".cfi_escape 0x10,0x0d,0x06,0x76,0x10,0x06,0x23,0xe0,0x01\n\t") /* %r13   */
                   __ASM_CFI(".cfi_escape 0x10,0x0e,0x06,0x76,0x10,0x06,0x23,0xe8,0x01\n\t") /* %r14   */
                   __ASM_CFI(".cfi_escape 0x10,0x0f,0x06,0x76,0x10,0x06,0x23,0xf0,0x01\n\t") /* %r15   */
                   __ASM_CFI(".cfi_escape 0x10,0x10,0x06,0x76,0x10,0x06,0x23,0xf8,0x01\n\t") /* %rip   */
                   __ASM_CFI(".cfi_escape 0x10,0x17,0x06,0x76,0x10,0x06,0x23,0x80,0x04\n\t") /* %xmm6  */
                   __ASM_CFI(".cfi_escape 0x10,0x18,0x06,0x76,0x10,0x06,0x23,0x90,0x04\n\t") /* %xmm7  */
                   __ASM_CFI(".cfi_escape 0x10,0x19,0x06,0x76,0x10,0x06,0x23,0xa0,0x04\n\t") /* %xmm8  */
                   __ASM_CFI(".cfi_escape 0x10,0x1a,0x06,0x76,0x10,0x06,0x23,0xb0,0x04\n\t") /* %xmm9  */
                   __ASM_CFI(".cfi_escape 0x10,0x1b,0x06,0x76,0x10,0x06,0x23,0xc0,0x04\n\t") /* %xmm10 */
                   __ASM_CFI(".cfi_escape 0x10,0x1c,0x06,0x76,0x10,0x06,0x23,0xd0,0x04\n\t") /* %xmm11 */
                   __ASM_CFI(".cfi_escape 0x10,0x1d,0x06,0x76,0x10,0x06,0x23,0xe0,0x04\n\t") /* %xmm12 */
                   __ASM_CFI(".cfi_escape 0x10,0x1e,0x06,0x76,0x10,0x06,0x23,0xf0,0x04\n\t") /* %xmm13 */
                   __ASM_CFI(".cfi_escape 0x10,0x1f,0x06,0x76,0x10,0x06,0x23,0x80,0x05\n\t") /* %xmm14 */
                   __ASM_CFI(".cfi_escape 0x10,0x20,0x06,0x76,0x10,0x06,0x23,0x90,0x05\n\t") /* %xmm15 */
                   "movq %r8,%rcx\n\t"
                   "callq *%rdx\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
                   "movq %rbp,%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret");

/* layering violation: the setjmp buffer is defined in msvcrt, but used by RtlUnwindEx */
struct MSVCRT_JUMP_BUFFER
{
    ULONG64 Frame;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Rip;
    ULONG64 Spare;
    M128A   Xmm6;
    M128A   Xmm7;
    M128A   Xmm8;
    M128A   Xmm9;
    M128A   Xmm10;
    M128A   Xmm11;
    M128A   Xmm12;
    M128A   Xmm13;
    M128A   Xmm14;
    M128A   Xmm15;
};

void WINAPI ntdll_RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    struct qemu_syscall call;

    /* For logging. */
    call.id = QEMU_SYSCALL_ID(CALL_RTLRESTORECONTEXT);
    qemu_syscall(&call);

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct MSVCRT_JUMP_BUFFER *jmp = (struct MSVCRT_JUMP_BUFFER *)rec->ExceptionInformation[0];
        context->Rbx   = jmp->Rbx;
        context->Rsp   = jmp->Rsp;
        context->Rbp   = jmp->Rbp;
        context->Rsi   = jmp->Rsi;
        context->Rdi   = jmp->Rdi;
        context->R12   = jmp->R12;
        context->R13   = jmp->R13;
        context->R14   = jmp->R14;
        context->R15   = jmp->R15;
        context->Rip   = jmp->Rip;
        context->Xmm6  = jmp->Xmm6;
        context->Xmm7  = jmp->Xmm7;
        context->Xmm8  = jmp->Xmm8;
        context->Xmm9  = jmp->Xmm9;
        context->Xmm10 = jmp->Xmm10;
        context->Xmm11 = jmp->Xmm11;
        context->Xmm12 = jmp->Xmm12;
        context->Xmm13 = jmp->Xmm13;
        context->Xmm14 = jmp->Xmm14;
        context->Xmm15 = jmp->Xmm15;
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        context->Rip = (ULONG64)call_consolidate_callback( context, consolidate, rec );
    }

    set_cpu_context( context );
}

/* Copypasted from Wine. */
void WINAPI ntdll_RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT new_context;
    NTSTATUS status;
    DWORD i;
    struct qemu_ExceptDebug call;

    ntdll_RtlCaptureContext( context );
    new_context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Rip;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EH_UNWINDING | (end_frame ? 0 : EH_EXIT_UNWIND);

    call.super.id = QEMU_SYSCALL_ID(CALL_RTLUNWINDEX);
    call.string = (ULONG_PTR)"code=%lx flags=%lx end_frame=%p target_ip=%p rip=%016lx\n";
    call.p1 = rec->ExceptionCode;
    call.p2 = rec->ExceptionFlags;
    call.p3 = (ULONG_PTR)end_frame;
    call.p4 = (ULONG_PTR)target_ip;
    call.p5 = context->Rip;
    call.num_params = 5;

    /*
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%d]=%016lx\n", i, rec->ExceptionInformation[i] );
    TRACE(" rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
          context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE(" rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
          context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE("  r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
          context->R8, context->R9, context->R10, context->R11 );
    TRACE(" r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
          context->R12, context->R13, context->R14, context->R15 );*/

    dispatch.EstablisherFrame = context->Rsp;
    dispatch.TargetIp         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;

    for (;;)
    {
        /* FIXME: should use the history table to make things faster */

        dispatch.ImageBase = 0;
        dispatch.ScopeIndex = 0;
        dispatch.ControlPc = context->Rip;

        /* first look for PE exception information */

        if ((dispatch.FunctionEntry = ntdll_RtlLookupFunctionEntry( context->Rip, &dispatch.ImageBase, NULL )))
        {
            dispatch.LanguageHandler = ntdll_RtlVirtualUnwind( UNW_FLAG_UHANDLER, dispatch.ImageBase,
                                                         context->Rip, dispatch.FunctionEntry,
                                                         &new_context, &dispatch.HandlerData,
                                                         &dispatch.EstablisherFrame, NULL );
            call.string = (ULONG_PTR)"Found handler %p in function entry %p (pc %p).\n";
            call.p1 = (ULONG_PTR)dispatch.LanguageHandler;
            call.p2 = (ULONG_PTR)dispatch.FunctionEntry;
            call.p3 = (ULONG_PTR)context->Rip;
            call.num_params = 3;
            qemu_syscall(&call.super);

            goto unwind_done;
        }

        /* no exception information, treat as a leaf function */

        new_context.Rip = *(ULONG64 *)context->Rsp;
        new_context.Rsp = context->Rsp + sizeof(ULONG64);
        dispatch.EstablisherFrame = new_context.Rsp;
        dispatch.LanguageHandler = NULL;

        call.string = (ULONG_PTR)"Leaf function Rip=%p, Rsp=%p.\n";
        call.p1 = (ULONG_PTR)new_context.Rip;
        call.p2 = (ULONG_PTR)new_context.Rsp;
        call.num_params = 2;
        qemu_syscall(&call.super);

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (is_inside_signal_stack( (void *)dispatch.EstablisherFrame ))
        {
            call.string = (ULONG_PTR)"frame %lx is inside signal stack (%p-%p)\n";
            call.p1 = dispatch.EstablisherFrame;
            call.p2 = (ULONG_PTR)get_signal_stack();
            call.p3 = (ULONG_PTR)get_signal_stack() + signal_stack_size;
            call.num_params = 3;
            qemu_syscall(&call.super);

            *context = new_context;
            continue;
        }

        if ((dispatch.EstablisherFrame & 7) ||
            dispatch.EstablisherFrame < (ULONG64)((NT_TIB *)NtCurrentTeb())->StackLimit ||
            dispatch.EstablisherFrame > (ULONG64)((NT_TIB *)NtCurrentTeb())->StackBase)
        {
            /*ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );*/
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                call.string = (ULONG_PTR)"invalid end frame %lx/%p\n";
                call.p1 = dispatch.EstablisherFrame;
                call.p2 = (ULONG_PTR)end_frame;
                call.num_params = 2;
                qemu_syscall(&call.super);

                /*raise_status( STATUS_INVALID_UNWIND_TARGET, rec );*/
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EH_TARGET_UNWIND;
            if (call_unwind_handler( rec, &dispatch ) == ExceptionCollidedUnwind)
            {
                ULONG64 frame;

                *context = new_context = *dispatch.ContextRecord;
                dispatch.ContextRecord = context;
                ntdll_RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &new_context, NULL, &frame, NULL );
                rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                goto unwind_done;
            }
            rec->ExceptionFlags &= ~EH_COLLIDED_UNWIND;
        }

        call.string = (ULONG_PTR)"Current frame %lx, searching for end frame %lx\n";
        call.p1 = dispatch.EstablisherFrame;
        call.p2 = (ULONG_PTR)end_frame;
        call.num_params = 2;
        qemu_syscall(&call.super);

        if (dispatch.EstablisherFrame == (ULONG64)end_frame) break;
        *context = new_context;
    }

    call.string = (ULONG_PTR)"All done, jumping to %p\n";
    call.p1 = (ULONG_PTR)target_ip;
    call.num_params = 1;
    qemu_syscall(&call.super);

    context->Rax = (ULONG64)retval;
    context->Rip = (ULONG64)target_ip;
    ntdll_RtlRestoreContext(context, rec);
}

void WINAPI RtlUnwind( void *frame, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    ntdll_RtlUnwindEx( frame, target_ip, rec, retval, &context, NULL );
}

/* Copypasted from Wine. */
NTSYSAPI EXCEPTION_DISPOSITION WINAPI __C_specific_handler(EXCEPTION_RECORD *rec,
        ULONG64 frame, CONTEXT *context, struct _DISPATCHER_CONTEXT *dispatch)
{
    SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG i;
    struct qemu_ExceptDebug call;

    call.super.id = QEMU_SYSCALL_ID(CALL___C_SPECIFIC_HANDLER);
    call.string = (ULONG_PTR)"Record %p, frame %p, context %p, dispatch %p.\n";
    call.p1 = (ULONG_PTR)rec;
    call.p2 = frame;
    call.p3 = (ULONG_PTR)context;
    call.p4 = (ULONG_PTR)dispatch;
    call.num_params = 4;
    qemu_syscall(&call.super);

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        call.string = (ULONG_PTR)"Unwinding.\n";
        call.num_params = 0;
        qemu_syscall(&call.super);

        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (dispatch->ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                dispatch->ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
            {
                TERMINATION_HANDLER handler;

                if (table->ScopeRecord[i].JumpTarget) continue;

                if (rec->ExceptionFlags & EH_TARGET_UNWIND &&
                    dispatch->TargetIp >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                    dispatch->TargetIp < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
                {
                    break;
                }

                handler = (TERMINATION_HANDLER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                dispatch->ScopeIndex = i+1;

                call.string = (ULONG_PTR)"calling __finally %p frame %lx\n";
                call.p1 = (ULONG_PTR)handler;
                call.p2 = frame;
                call.num_params = 2;
                qemu_syscall(&call.super);

                handler( 1, frame );
            }
        }
        return ExceptionContinueSearch;
    }

    for (i = dispatch->ScopeIndex; i < table->Count; i++)
    {
        if (dispatch->ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
            dispatch->ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
        {
            if (!table->ScopeRecord[i].JumpTarget) continue;
            if (table->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
            {
                EXCEPTION_POINTERS ptrs;
                PC_LANGUAGE_EXCEPTION_HANDLER filter;

                filter = (PC_LANGUAGE_EXCEPTION_HANDLER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                ptrs.ExceptionRecord = rec;
                ptrs.ContextRecord = context;

                call.string = (ULONG_PTR)"calling filter %p ptrs %p frame %lx\n";
                call.p1 = (ULONG_PTR)filter;
                call.p2 = (ULONG_PTR)&ptrs;
                call.p3 = frame;
                call.num_params = 3;
                qemu_syscall(&call.super);

                call.num_params = 0;
                switch (filter( &ptrs, frame ))
                {
                case EXCEPTION_EXECUTE_HANDLER:
                    call.string = (ULONG_PTR)"Filter returned EXCEPTION_EXECUTE_HANDLER\n";
                    qemu_syscall(&call.super);
                    break;
                case EXCEPTION_CONTINUE_SEARCH:
                    call.string = (ULONG_PTR)"Filter returned EXCEPTION_CONTINUE_SEARCH\n";
                    qemu_syscall(&call.super);
                    continue;
                case EXCEPTION_CONTINUE_EXECUTION:
                    call.string = (ULONG_PTR)"Filter returned EXCEPTION_CONTINUE_EXECUTION\n";
                    qemu_syscall(&call.super);
                    return ExceptionContinueExecution;
                }
            }

            call.string = (ULONG_PTR)"unwinding to target %lx, end frame %lx\n";
            call.p1 = dispatch->ImageBase + table->ScopeRecord[i].JumpTarget;
            call.p2 = frame;
            call.num_params = 2;
            qemu_syscall(&call.super);

            ntdll_RtlUnwindEx( (void *)frame, (char *)dispatch->ImageBase + table->ScopeRecord[i].JumpTarget,
                         rec, 0, dispatch->ContextRecord, dispatch->HistoryTable );
        }
    }
    return ExceptionContinueSearch;

}

#else

#define __ASM_CFI(str) str
#define __ASM_DEFINE_FUNC(name,suffix,code) asm(".text\n\t.align 4\n\t.globl _" #name suffix "\n\t.def _" #name suffix "; .scl 2; .type 32; .endef\n_" #name suffix ":\n\t.cfi_startproc\n\t" code "\n\t.cfi_endproc");
#define __ASM_STDCALL(args) "@" #args
#define __ASM_GLOBAL_FUNC(name,code) __ASM_DEFINE_FUNC(name,"",code)
#define __ASM_NAME(name) "_" name
#define __ASM_STDCALL_FUNC(name,args,code) __ASM_DEFINE_FUNC(name,__ASM_STDCALL(args),code)

extern VOID NTAPI ntdll_RtlCaptureContext(PCONTEXT ContextRecord);
__ASM_STDCALL_FUNC( ntdll_RtlCaptureContext, 4,
                    "pushl %eax\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "movl 8(%esp),%eax\n\t"    /* context */
                    "movl $0x10007,(%eax)\n\t" /* context->ContextFlags */
                    "movw %gs,0x8c(%eax)\n\t"  /* context->SegGs */
                    "movw %fs,0x90(%eax)\n\t"  /* context->SegFs */
                    "movw %es,0x94(%eax)\n\t"  /* context->SegEs */
                    "movw %ds,0x98(%eax)\n\t"  /* context->SegDs */
                    "movl %edi,0x9c(%eax)\n\t" /* context->Edi */
                    "movl %esi,0xa0(%eax)\n\t" /* context->Esi */
                    "movl %ebx,0xa4(%eax)\n\t" /* context->Ebx */
                    "movl %edx,0xa8(%eax)\n\t" /* context->Edx */
                    "movl %ecx,0xac(%eax)\n\t" /* context->Ecx */
                    "movl 0(%ebp),%edx\n\t"
                    "movl %edx,0xb4(%eax)\n\t" /* context->Ebp */
                    "movl 4(%ebp),%edx\n\t"
                    "movl %edx,0xb8(%eax)\n\t" /* context->Eip */
                    "movw %cs,0xbc(%eax)\n\t"  /* context->SegCs */
                    "pushfl\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "popl 0xc0(%eax)\n\t"      /* context->EFlags */
                    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                    "leal 8(%ebp),%edx\n\t"
                    "movl %edx,0xc4(%eax)\n\t" /* context->Esp */
                    "movw %ss,0xc8(%eax)\n\t"  /* context->SegSs */
                    "popl 0xb0(%eax)\n\t"      /* context->Eax */
                    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                    "ret $4" )

static void raise_status(NTSTATUS status, EXCEPTION_RECORD *record)
{
    struct qemu_ExceptDebug call;
    call.super.id = QEMU_SYSCALL_ID(CALL_RTLUNWINDEX);
    call.string = (ULONG_PTR)"Bad status=%lx\n";
    call.p1 = (ULONG_PTR)status;
    call.num_params = 1;

    qemu_syscall(&call.super);
}

typedef struct
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_REGISTRATION_RECORD *prevFrame;
} EXC_NESTED_FRAME;

extern DWORD EXC_CallHandler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher,
                              PEXCEPTION_ROUTINE handler, PEXCEPTION_HANDLER nested_handler );

static inline BOOL is_valid_frame( void *frame )
{
    if ((ULONG_PTR)frame & 3) return FALSE;
    return (frame >= ((NT_TIB *)NtCurrentTeb())->StackLimit &&
            (void **)frame < (void **)((NT_TIB *)NtCurrentTeb())->StackBase - 1);
}

static int unwind_handler( EXCEPTION_RECORD *rec, void *shutup,
                             CONTEXT *context, void *compiler )
{
    EXCEPTION_REGISTRATION_RECORD *frame = shutup;
    EXCEPTION_REGISTRATION_RECORD **dispatcher = compiler;

    if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionCollidedUnwind;
}

static inline EXCEPTION_REGISTRATION_RECORD *__wine_pop_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
#if defined(__GNUC__) && defined(__i386__)
    __asm__ __volatile__(".byte 0x64\n\tmovl %0,(0)"
                         : : "r" (frame->prev) : "memory" );
    return frame->prev;

#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    teb->ExceptionList = frame->prev;
    return frame->prev;
#endif
}

NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context );

void WINAPI __regs_RtlUnwind( EXCEPTION_REGISTRATION_RECORD* pEndFrame, PVOID targetIp,
                                              PEXCEPTION_RECORD pRecord, PVOID retval, CONTEXT *context )
{
    EXCEPTION_RECORD record;
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch;
    DWORD res;
    struct qemu_ExceptDebug call;

    call.super.id = QEMU_SYSCALL_ID(CALL_RTLUNWINDEX);
    call.string = (ULONG_PTR)"Frame=%lx, target=%lx, exception %lx, retval %lx\n";
    call.p1 = (ULONG_PTR)frame;
    call.p2 = (ULONG_PTR)targetIp;
    call.p3 = (ULONG_PTR)pRecord;
    call.p4 = (ULONG_PTR)retval;
    call.num_params = 4;

    qemu_syscall(&call.super);

    context->Eax = (DWORD)retval;

    /* build an exception record, if we do not have one */
    if (!pRecord)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Eip;
        record.NumberParameters = 0;
        pRecord = &record;
    }

    pRecord->ExceptionFlags |= EH_UNWINDING | (pEndFrame ? 0 : EH_EXIT_UNWIND);

    /*TRACE( "code=%x flags=%x\n", pRecord->ExceptionCode, pRecord->ExceptionFlags );
    TRACE( "eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
           context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi, context->Edi );
    TRACE( "ebp=%08x esp=%08x eip=%08x cs=%04x ds=%04x fs=%04x gs=%04x flags=%08x\n",
           context->Ebp, context->Esp, context->Eip, LOWORD(context->SegCs), LOWORD(context->SegDs),
           LOWORD(context->SegFs), LOWORD(context->SegGs), context->EFlags );*/

    /* get chain of exception frames */
    frame = ((NT_TIB *)NtCurrentTeb())->ExceptionList;
    while ((frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL) && (frame != pEndFrame))
    {
        /* Check frame address */
        if (pEndFrame && (frame > pEndFrame))
            raise_status( STATUS_INVALID_UNWIND_TARGET, pRecord );

        if (!is_valid_frame( frame )) raise_status( STATUS_BAD_STACK, pRecord );

        /* Call handler */
        /*TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, pRecord->ExceptionCode, pRecord->ExceptionFlags );*/
        res = EXC_CallHandler( pRecord, frame, context, &dispatch, frame->Handler, unwind_handler );
        /*TRACE( "handler at %p returned %x\n", frame->Handler, res );*/

        switch(res)
        {
        case ExceptionContinueSearch:
            break;
        case ExceptionCollidedUnwind:
            frame = dispatch;
            break;
        default:
            raise_status( STATUS_INVALID_DISPOSITION, pRecord );
            break;
        }
        frame = __wine_pop_frame( frame );
    }

    /* FIXME: It seems that GetCurrentThread() is a function in mingw and not a macro as Wine's code expects. */
    NtSetContextThread( ((HANDLE)~(ULONG_PTR)1), context );
}
__ASM_STDCALL_FUNC( RtlUnwind, 16,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "leal -(0x2cc+8)(%esp),%esp\n\t" /* sizeof(CONTEXT) + alignment */
                    "pushl %eax\n\t"
                    "leal 4(%esp),%eax\n\t"          /* context */
                    "xchgl %eax,(%esp)\n\t"
                    "call " __ASM_NAME("ntdll_RtlCaptureContext") __ASM_STDCALL(4) "\n\t"
                    "leal 24(%ebp),%eax\n\t"
                    "movl %eax,0xc4(%esp)\n\t"       /* context->Esp */
                    "pushl %esp\n\t"
                    "pushl 20(%ebp)\n\t"
                    "pushl 16(%ebp)\n\t"
                    "pushl 12(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call " __ASM_NAME("__regs_RtlUnwind") __ASM_STDCALL(20) "\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret $16" )  /* actually never returns */

#endif /* _WIN64 */

#else

void qemu_RtlRestoreContext(struct qemu_syscall *call)
{
    WINE_TRACE("\n");
}

void qemu_RtlUnwindEx(struct qemu_syscall *call)
{
    struct qemu_ExceptDebug *c = (struct qemu_ExceptDebug *)call;

    switch (c->num_params)
    {
        case 5:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4, c->p5);
            break;
        case 4:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4);
            break;
        case 3:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3);
            break;
        case 2:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2);
            break;
        case 1:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1);
            break;
        case 0:
            WINE_TRACE((const char *)QEMU_G2H(c->string), 0); /* Hmm, why do I need one arg? */
            break;
    }

    c->super.iret = WINE_TRACE_ON(qemu_ntdll);
}

void qemu___C_specific_handler(struct qemu_syscall *call)
{
    struct qemu_ExceptDebug *c = (struct qemu_ExceptDebug *)call;

    switch (c->num_params)
    {
        case 5:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4, c->p5);
            break;
        case 4:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4);
            break;
        case 3:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3);
            break;
        case 2:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2);
            break;
        case 1:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1);
            break;
        case 0:
            WINE_TRACE((const char *)QEMU_G2H(c->string), 0); /* Hmm, why do I need one arg? */
            break;
    }

    c->super.iret = WINE_TRACE_ON(qemu_ntdll);
}

#endif

struct qemu_RtlAddFunctionTable
{
    struct qemu_syscall super;
    uint64_t func;
    uint64_t entry_count;
    uint64_t base;
};


#ifdef QEMU_DLL_GUEST

#ifdef _WIN64
__ASM_STDCALL_FUNC( DbgBreakPoint, 0, "int $3; ret");
#else
void WINAPI DbgBreakPoint(void)
{
}
#endif

#endif

#ifdef QEMU_DLL_GUEST

#ifdef _WIN64
NTSYSAPI BOOLEAN CDECL RtlAddFunctionTable(PRUNTIME_FUNCTION func, DWORD entry_count, DWORD64 base)
{
    struct qemu_RtlAddFunctionTable call;
    call.super.id = QEMU_SYSCALL_ID(CALL_RTLADDFUNCTIONTABLE);
    call.func = (ULONG_PTR)func;
    call.entry_count = entry_count;
    call.base = base;

    qemu_syscall(&call.super);

    return call.super.iret;
}
#endif

#else

void qemu_RtlAddFunctionTable(struct qemu_syscall *call)
{
    struct qemu_RtlAddFunctionTable *c = (struct qemu_RtlAddFunctionTable *)call;
    WINE_FIXME("(%p, %lu, %#lx) Stub!\n", QEMU_G2H(c->func), (unsigned long)c->entry_count, (unsigned long)c->base);
    c->super.iret = TRUE;
}

#endif

#ifdef QEMU_DLL_GUEST

#ifdef _WIN64
static RUNTIME_FUNCTION *find_function_info( ULONG64 pc, HMODULE module,
                                             RUNTIME_FUNCTION *func, ULONG size )
{
    int min = 0;
    int max = size/sizeof(*func) - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if ((char *)pc < (char *)module + func[pos].BeginAddress) max = pos - 1;
        else if ((char *)pc >= (char *)module + func[pos].EndAddress) min = pos + 1;
        else
        {
            func += pos;
            while (func->UnwindData & 1)  /* follow chained entry */
                func = (RUNTIME_FUNCTION *)((char *)module + (func->UnwindData & ~1));
            return func;
        }
    }
    return NULL;
}

PRUNTIME_FUNCTION NTAPI ntdll_RtlLookupFunctionEntry(DWORD64 pc, DWORD64 *base, UNWIND_HISTORY_TABLE *history)
{
    struct qemu_syscall call;
    LDR_MODULE *module;
    ULONG size;
    RUNTIME_FUNCTION *func = NULL;

    /* For tracing. */
    call.id = QEMU_SYSCALL_ID(CALL_RTLLOOKUPFUNCTIONENTRY);

    qemu_syscall(&call);

    if (!ntdll_LdrFindEntryForAddress( (void *)pc, &module))
    {
        *base = (DWORD64)module;
        if ((func = ntdll_RtlImageDirectoryEntryToData(module, TRUE,
                IMAGE_DIRECTORY_ENTRY_EXCEPTION, &size)))
        {
            /* lookup in function table */
            func = find_function_info(pc, module, func, size);
        }
    }
    else
    {
        /* No support for dynamic call tables yet. */
    }

    return func;
}

#endif /* _WIN64 */

#else

void qemu_RtlLookupFunctionEntry(struct qemu_syscall *call)
{
    WINE_TRACE("\n");
}

#endif

#ifdef QEMU_DLL_GUEST

#ifdef _WIN64
union handler_data
{
    RUNTIME_FUNCTION chain;
    ULONG handler;
};

struct opcode
{
    BYTE offset;
    BYTE code : 4;
    BYTE info : 4;
};

struct UNWIND_INFO
{
    BYTE version : 3;
    BYTE flags : 5;
    BYTE prolog;
    BYTE count;
    BYTE frame_reg : 4;
    BYTE frame_offset : 4;
    struct opcode opcodes[1];  /* info->count entries */
    /* followed by handler_data */
};

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

static BOOL is_inside_epilog( BYTE *pc, ULONG64 base, const RUNTIME_FUNCTION *function )
{
    /* add or lea must be the first instruction, and it must have a rex.W prefix */
    if ((pc[0] & 0xf8) == 0x48)
    {
        switch (pc[1])
        {
        case 0x81: /* add $nnnn,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 7;
                break;
            }
            return FALSE;
        case 0x83: /* add $n,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 4;
                break;
            }
            return FALSE;
        case 0x8d: /* lea n(reg),%rsp */
            if (pc[0] & 0x06) return FALSE;  /* rex.RX must be cleared */
            if (((pc[2] >> 3) & 7) != 4) return FALSE;  /* dest reg mus be %rsp */
            if ((pc[2] & 7) == 4) return FALSE;  /* no SIB byte allowed */
            if ((pc[2] >> 6) == 1)  /* 8-bit offset */
            {
                pc += 4;
                break;
            }
            if ((pc[2] >> 6) == 2)  /* 32-bit offset */
            {
                pc += 7;
                break;
            }
            return FALSE;
        }
    }

    /* now check for various pop instructions */

    for (;;)
    {
        if ((*pc & 0xf0) == 0x40) pc++;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/%r8 */
        case 0x59: /* pop %rcx/%r9 */
        case 0x5a: /* pop %rdx/%r10 */
        case 0x5b: /* pop %rbx/%r11 */
        case 0x5c: /* pop %rsp/%r12 */
        case 0x5d: /* pop %rbp/%r13 */
        case 0x5e: /* pop %rsi/%r14 */
        case 0x5f: /* pop %rdi/%r15 */
            pc++;
            continue;
        case 0xc2: /* ret $nn */
        case 0xc3: /* ret */
            return TRUE;
        case 0xe9: /* jmp nnnn */
            pc += 5 + *(LONG *)(pc + 1);
            if (pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress)
                continue;
            break;
        case 0xeb: /* jmp n */
            pc += 2 + (signed char)pc[1];
            if (pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress)
                continue;
            break;
        case 0xf3: /* rep; ret (for amd64 prediction bug) */
            return pc[1] == 0xc3;
        }
        return FALSE;
    }
}
static ULONG64 get_int_reg( CONTEXT *context, int reg )
{
    return *(&context->Rax + reg);
}

static void set_int_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, ULONG64 *val )
{
    *(&context->Rax + reg) = *val;
    if (ctx_ptr) ctx_ptr->IntegerContext[reg] = val;
}

static void set_float_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, M128A *val )
{
    *(&context->Xmm0 + reg) = *val;
    if (ctx_ptr) ctx_ptr->FloatingContext[reg] = val;
}

static int get_opcode_size( struct opcode op )
{
    switch (op.code)
    {
    case UWOP_ALLOC_LARGE:
        return 2 + (op.info != 0);
    case UWOP_SAVE_NONVOL:
    case UWOP_SAVE_XMM128:
        return 2;
    case UWOP_SAVE_NONVOL_FAR:
    case UWOP_SAVE_XMM128_FAR:
        return 3;
    default:
        return 1;
    }
}

/* execute a function epilog, which must have been validated with is_inside_epilog() */
static void interpret_epilog( BYTE *pc, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    for (;;)
    {
        BYTE rex = 0;

        if ((*pc & 0xf0) == 0x40) rex = *pc++ & 0x0f;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/r8 */
        case 0x59: /* pop %rcx/r9 */
        case 0x5a: /* pop %rdx/r10 */
        case 0x5b: /* pop %rbx/r11 */
        case 0x5c: /* pop %rsp/r12 */
        case 0x5d: /* pop %rbp/r13 */
        case 0x5e: /* pop %rsi/r14 */
        case 0x5f: /* pop %rdi/r15 */
            set_int_reg( context, ctx_ptr, *pc - 0x58 + (rex & 1) * 8, (ULONG64 *)context->Rsp );
            context->Rsp += sizeof(ULONG64);
            pc++;
            continue;
        case 0x81: /* add $nnnn,%rsp */
            context->Rsp += *(LONG *)(pc + 2);
            pc += 2 + sizeof(LONG);
            continue;
        case 0x83: /* add $n,%rsp */
            context->Rsp += (signed char)pc[2];
            pc += 3;
            continue;
        case 0x8d:
            if ((pc[1] >> 6) == 1)  /* lea n(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + (signed char)pc[2];
                pc += 3;
            }
            else  /* lea nnnn(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + *(LONG *)(pc + 2);
                pc += 2 + sizeof(LONG);
            }
            continue;
        case 0xc2: /* ret $nn */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64) + *(WORD *)(pc + 1);
            return;
        case 0xc3: /* ret */
        case 0xf3: /* rep; ret */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64);
            return;
        case 0xe9: /* jmp nnnn */
            pc += 5 + *(LONG *)(pc + 1);
            continue;
        case 0xeb: /* jmp n */
            pc += 2 + (signed char)pc[1];
            continue;
        }
        return;
    }
}

/* Copypasted from Wine with minor adjustments for mingw's headers. */
PEXCEPTION_ROUTINE WINAPI ntdll_RtlVirtualUnwind(DWORD type, DWORD64 base, DWORD64 pc,
        PRUNTIME_FUNCTION function, PCONTEXT context, PVOID *data, PDWORD64 frame_ret,
        PKNONVOLATILE_CONTEXT_POINTERS ctx_ptr)
{
    union handler_data *handler_data;
    ULONG64 frame, off;
    struct UNWIND_INFO *info;
    unsigned int i, prolog_offset;
    struct qemu_ExceptDebug call;

    call.super.id = QEMU_SYSCALL_ID(CALL_RTLVIRTUALUNWIND);
    call.string = (ULONG_PTR)"PC %p, start frame %p\n";
    call.p1 = (ULONG_PTR)pc;
    call.p2 = context->Rsp;
    call.num_params = 2;
    qemu_syscall(&call.super);

    frame = *frame_ret = context->Rsp;
    for (;;)
    {
        info = (struct UNWIND_INFO *)((char *)base + function->UnwindData);
        handler_data = (union handler_data *)&info->opcodes[(info->count + 1) & ~1];

        if (info->version != 1)
        {
            call.super.id = QEMU_SYSCALL_ID(CALL_RTLVIRTUALUNWIND);
            call.string = (ULONG_PTR)"unknown unwind info version %u at %p\n";
            call.p1 = info->version;
            call.p2 = (ULONG_PTR)info;
            call.num_params = 2;
            qemu_syscall(&call.super);
            return NULL;
        }

        if (info->frame_reg)
            frame = get_int_reg( context, info->frame_reg ) - info->frame_offset * 16;

        /* check if in prolog */
        if (pc >= base + function->BeginAddress && pc < base + function->BeginAddress + info->prolog)
        {
            prolog_offset = pc - base - function->BeginAddress;
        }
        else
        {
            prolog_offset = ~0;
            if (is_inside_epilog( (BYTE *)pc, base, function ))
            {
                interpret_epilog( (BYTE *)pc, context, ctx_ptr );
                *frame_ret = frame;

                call.string = (ULONG_PTR)"In epilog? %p\n";
                call.p1 = (ULONG_PTR)pc;
                call.num_params = 1;
                qemu_syscall(&call.super);

                return NULL;
            }
        }

        for (i = 0; i < info->count; i += get_opcode_size(info->opcodes[i]))
        {
            if (prolog_offset < info->opcodes[i].offset) continue; /* skip it */

            switch (info->opcodes[i].code)
            {
            case UWOP_PUSH_NONVOL:  /* pushq %reg */
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)context->Rsp );
                context->Rsp += sizeof(ULONG64);
                break;
            case UWOP_ALLOC_LARGE:  /* subq $nn,%rsp */
                if (info->opcodes[i].info) context->Rsp += *(DWORD *)&info->opcodes[i+1];
                else context->Rsp += *(USHORT *)&info->opcodes[i+1] * 8;
                break;
            case UWOP_ALLOC_SMALL:  /* subq $n,%rsp */
                context->Rsp += (info->opcodes[i].info + 1) * 8;
                break;
            case UWOP_SET_FPREG:  /* leaq nn(%rsp),%framereg */
                context->Rsp = *frame_ret = frame;
                break;
            case UWOP_SAVE_NONVOL:  /* movq %reg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 8;
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_NONVOL_FAR:  /* movq %reg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_XMM128:  /* movaps %xmmreg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 16;
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_SAVE_XMM128_FAR:  /* movaps %xmmreg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_PUSH_MACHFRAME:
                /*FIXME( "PUSH_MACHFRAME %u\n", info->opcodes[i].info );*/
                break;
            default:
                /*FIXME( "unknown code %u\n", info->opcodes[i].code );*/
                break;
            }
        }

        if (!(info->flags & UNW_FLAG_CHAININFO)) break;
        function = &handler_data->chain;  /* restart with the chained info */
    }

    /* now pop return address */
    context->Rip = *(ULONG64 *)context->Rsp;
    context->Rsp += sizeof(ULONG64);

    if (!(info->flags & type))
    {
        call.string = (ULONG_PTR)"No matching handler?\n";
        call.num_params = 0;
        qemu_syscall(&call.super);

        return NULL;  /* no matching handler */
    }
    if (prolog_offset != ~0)
    {
        call.string = (ULONG_PTR)"In prolog?\n";
        call.num_params = 0;
        qemu_syscall(&call.super);

        return NULL;  /* inside prolog */
    }

    call.string = (ULONG_PTR)"Found function %p, EstablisherFrame %p.\n";
    call.p1 = (ULONG_PTR)((char *)base + handler_data->handler);
    call.p2 = *frame_ret;
    call.num_params = 2;
    qemu_syscall(&call.super);

    *data = &handler_data->handler + 1;
    return (EXCEPTION_ROUTINE*)((char *)base + handler_data->handler);
}

#else
#endif /* _WIN64 */

#else

void qemu_RtlVirtualUnwind(struct qemu_syscall *call)
{
    struct qemu_ExceptDebug *c = (struct qemu_ExceptDebug *)call;

    switch (c->num_params)
    {
        case 5:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4, c->p5);
            break;
        case 4:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4);
            break;
        case 3:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3);
            break;
        case 2:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2);
            break;
        case 1:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1);
            break;
        case 0:
            WINE_TRACE((const char *)QEMU_G2H(c->string), 0); /* Hmm, why do I need one arg? */
            break;
    }

    c->super.iret = WINE_TRACE_ON(qemu_ntdll);
}

#endif

struct qemu_NtSetContextThread
{
    struct qemu_syscall super;
    uint64_t handle;
    uint64_t context;
};

#ifdef QEMU_DLL_GUEST

#ifdef _WIN64
static DWORD call_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    /* FIXME: Catch nested exceptions. */
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, context, dispatch );

    return res;
}

/* Copypasted from Wine. */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    LDR_MODULE *module;
    NTSTATUS status;
    struct qemu_ExceptDebug call;

    context = *orig_context;
    dispatch.TargetIp      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    call.super.id = QEMU_SYSCALL_ID(CALL_NTRAISEEXCEPTION);

    for (;;)
    {
        /* FIXME: should use the history table to make things faster */

        dispatch.ImageBase = 0;
        dispatch.ControlPc = context.Rip;
        dispatch.ScopeIndex = 0;

        /* first look for PE exception information */

        if ((dispatch.FunctionEntry = ntdll_RtlLookupFunctionEntry( dispatch.ControlPc, &dispatch.ImageBase, NULL )))
        {
            dispatch.LanguageHandler = ntdll_RtlVirtualUnwind( UNW_FLAG_EHANDLER, dispatch.ImageBase,
                                                         dispatch.ControlPc, dispatch.FunctionEntry,
                                                         &context, &dispatch.HandlerData,
                                                         &dispatch.EstablisherFrame, NULL );

            call.string = (ULONG_PTR)"Found handler %p in function entry %p(pc %p), frame %p.\n";
            call.p1 = (ULONG_PTR)dispatch.LanguageHandler;
            call.p2 = (ULONG_PTR)dispatch.FunctionEntry;
            call.p3 = (ULONG_PTR)dispatch.ControlPc;
            call.p4 = dispatch.EstablisherFrame;
            call.num_params = 4;
            qemu_syscall(&call.super);

            goto unwind_done;
        }

        context.Rip = *(ULONG64 *)context.Rsp;
        context.Rsp = context.Rsp + sizeof(ULONG64);
        dispatch.EstablisherFrame = context.Rsp;
        dispatch.LanguageHandler = NULL;

        call.string = (ULONG_PTR)"Leaf function Rip=%p, Rsp=%p.\n";
        call.p1 = (ULONG_PTR)context.Rip;
        call.p2 = (ULONG_PTR)context.Rsp;
        call.num_params = 2;
        qemu_syscall(&call.super);

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if ((dispatch.EstablisherFrame & 7) ||
            dispatch.EstablisherFrame < (ULONG64)((NT_TIB *)NtCurrentTeb())->StackLimit ||
            dispatch.EstablisherFrame > (ULONG64)((NT_TIB *)NtCurrentTeb())->StackBase)
        {
            call.string = (ULONG_PTR)"invalid frame %lx (%p-%p).\n";
            call.p1 = dispatch.EstablisherFrame;
            call.p2 = (ULONG_PTR)((NT_TIB *)NtCurrentTeb())->StackLimit;
            call.p3 = (ULONG_PTR)((NT_TIB *)NtCurrentTeb())->StackBase;
            call.num_params = 3;
            qemu_syscall(&call.super);

            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            DWORD ret;

            call.string = (ULONG_PTR)"Calling handler %p.\n";
            call.p1 = (ULONG_PTR)dispatch.LanguageHandler;
            call.num_params = 1;
            qemu_syscall(&call.super);

            ret = call_handler( rec, orig_context, &dispatch );

            call.string = (ULONG_PTR)"Handler returned %lu.\n";
            call.p1 = ret;
            call.num_params = 1;
            qemu_syscall(&call.super);

            switch (ret)
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                /*FIXME( "nested exception\n" );*/
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                ntdll_RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &context, NULL, &frame, NULL );
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }

        if (context.Rsp == (ULONG64)((NT_TIB *)NtCurrentTeb())->StackBase) break;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}

/* Copypasted from Wine. */
NTSTATUS WINAPI ntdll_NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status;
    struct qemu_ExceptDebug call;

    call.super.id = QEMU_SYSCALL_ID(CALL_NTRAISEEXCEPTION);
    call.string = (ULONG_PTR)"Exception %p, context %p, first chance %lu\n";
    call.p1 = (ULONG_PTR)rec;
    call.p2 = (ULONG_PTR)context;
    call.p3 = first_chance;
    call.num_params = 3;
    qemu_syscall(&call.super);

    if (first_chance)
    {
        DWORD c;

        /*TRACE( "code=%x flags=%x addr=%p ip=%lx tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->Rip, GetCurrentThreadId() );
        for (c = 0; c < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); c++)
            TRACE( " info[%d]=%016lx\n", c, rec->ExceptionInformation[c] );
        if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
        {
            if (rec->ExceptionInformation[1] >> 16)
                MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
            else
                MESSAGE( "wine: Call from %p to unimplemented function %s.%ld, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        }
        else
        {
            TRACE(" rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
                  context->Rax, context->Rbx, context->Rcx, context->Rdx );
            TRACE(" rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
                  context->Rsi, context->Rdi, context->Rbp, context->Rsp );
            TRACE("  r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
                  context->R8, context->R9, context->R10, context->R11 );
            TRACE(" r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
                  context->R12, context->R13, context->R14, context->R15 );
        }*/
        /*status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED) goto done;*/

        /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
        if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Rip--;

        /* TODO: Vectored exception handling
        if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION) goto done; */

        if ((status = call_stack_handlers( rec, context )) == STATUS_SUCCESS) goto done;
        if (status != STATUS_UNHANDLED_EXCEPTION) return status;
    }

    /* last chance exception */

    /* TODO: Debugger...
    status = send_debug_event( rec, FALSE, context );
    */
    if (/*status != DBG_CONTINUE*/1)
    {
        /*if (rec->ExceptionFlags & EH_STACK_INVALID)
            ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            ERR("Unhandled exception code %x flags %x addr %p\n",
                rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );*/
        ntdll_NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    }

done:
    set_cpu_context(context);
    return STATUS_SUCCESS;
}

#else

NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    struct qemu_NtSetContextThread call;
    /* We can't set debug registers ourselves, and we can't set a context on a different thread from here.
     * We need help from qemu in both cases. x86 Wine uses wineserver for this. */
    call.super.id = QEMU_SYSCALL_ID(CALL_NTSETCONTEXTTHREAD);
    call.handle = guest_HANDLE_g2h(handle);
    call.context = (ULONG_PTR)context;

    /* This won't return on the current thread, but it does return if it's a different context. */
    qemu_syscall(&call.super);

    return call.super.iret;
}

static int raise_handler( EXCEPTION_RECORD *rec, void *shutup,
                            CONTEXT *context, void *compiler )
{
    EXCEPTION_REGISTRATION_RECORD *frame = shutup;
    EXCEPTION_REGISTRATION_RECORD **dispatcher = compiler;
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionNestedException;
}

__ASM_GLOBAL_FUNC( EXC_CallHandler,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                   "movl 28(%ebp), %edx\n\t" /* ugly hack to pass the 6th param needed because of Shrinker */
                   "pushl 24(%ebp)\n\t"
                   "pushl 20(%ebp)\n\t"
                   "pushl 16(%ebp)\n\t"
                   "pushl 12(%ebp)\n\t"
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_NAME("call_exception_handler") "\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
__ASM_GLOBAL_FUNC(call_exception_handler,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $12,%esp\n\t"
                  "pushl 12(%ebp)\n\t"      /* make any exceptions in this... */
                  "pushl %edx\n\t"          /* handler be handled by... */
                  ".byte 0x64\n\t"
                  "pushl (0)\n\t"           /* nested_handler (passed in edx). */
                  ".byte 0x64\n\t"
                  "movl %esp,(0)\n\t"       /* push the new exception frame onto the exception stack. */
                  "pushl 20(%ebp)\n\t"
                  "pushl 16(%ebp)\n\t"
                  "pushl 12(%ebp)\n\t"
                  "pushl 8(%ebp)\n\t"
                  "movl 24(%ebp), %ecx\n\t" /* (*1) */
                  "call *%ecx\n\t"          /* call handler. (*2) */
                  ".byte 0x64\n\t"
                  "movl (0), %esp\n\t"      /* restore previous... (*3) */
                  ".byte 0x64\n\t"
                  "popl (0)\n\t"            /* exception frame. */
                  "movl %ebp, %esp\n\t"     /* restore saved stack, in case it was corrupted */
                  "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret $20" )            /* (*4) */

static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch, *nested_frame;
    DWORD res;
    struct qemu_ExceptDebug call;
    call.super.id = QEMU_SYSCALL_ID(CALL_NTRAISEEXCEPTION);

    frame = ((NT_TIB *)NtCurrentTeb())->ExceptionList;
    nested_frame = NULL;
    while (frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL)
    {
        /* Check frame address */
        if (!is_valid_frame( frame ))
        {
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        call.string = (ULONG_PTR)"calling handler at 0x%lx code=%lx flags=%lx\n";
        call.p1 = (ULONG_PTR)frame->Handler;
        call.p2 = rec->ExceptionCode;
        call.p3 = rec->ExceptionFlags;
        call.num_params = 3;

        /* Call handler */
        res = EXC_CallHandler( rec, frame, context, &dispatch, frame->Handler, raise_handler );

        call.string = (ULONG_PTR)"calling handler at 0x%lx code=%lx flags=%lx\n";
        call.p1 = (ULONG_PTR)frame->Handler;
        call.p2 = res;
        call.num_params = 2;

        if (frame == nested_frame)
        {
            /* no longer nested */
            nested_frame = NULL;
            rec->ExceptionFlags &= ~EH_NESTED_CALL;
        }

        switch(res)
        {
        case ExceptionContinueExecution:
            if (!(rec->ExceptionFlags & EH_NONCONTINUABLE)) return STATUS_SUCCESS;
            return STATUS_NONCONTINUABLE_EXCEPTION;
        case ExceptionContinueSearch:
            break;
        case ExceptionNestedException:
            if (nested_frame < dispatch) nested_frame = dispatch;
            rec->ExceptionFlags |= EH_NESTED_CALL;
            break;
        default:
            return STATUS_INVALID_DISPOSITION;
        }
        frame = frame->prev;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}

static NTSTATUS raise_exception( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status;
    struct qemu_ExceptDebug call;

    call.super.id = QEMU_SYSCALL_ID(CALL_NTRAISEEXCEPTION);

    if (first_chance)
    {
        DWORD c;

        call.string = (ULONG_PTR)"code=%lx flags=%lx addr=%lx ip=%08lx tid=%04lx\n";
        call.p1 = rec->ExceptionCode;
        call.p2 = rec->ExceptionFlags;
        call.p3 = (ULONG_PTR)rec->ExceptionAddress;
        call.p4 = context->Eip;
        call.p5 = 0x1234; /* FIXME, GetCurrentThreadId() */
        call.num_params = 5;
        qemu_syscall(&call.super);

        /*
        for (c = 0; c < rec->NumberParameters; c++)
            TRACE( " info[%d]=%08lx\n", c, rec->ExceptionInformation[c] );
        if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
        {
            if (rec->ExceptionInformation[1] >> 16)
                MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
            else
                MESSAGE( "wine: Call from %p to unimplemented function %s.%ld, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        }
        else
        {
            TRACE(" eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
                  context->Eax, context->Ebx, context->Ecx,
                  context->Edx, context->Esi, context->Edi );
            TRACE(" ebp=%08x esp=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
                  context->Ebp, context->Esp, context->SegCs, context->SegDs,
                  context->SegEs, context->SegFs, context->SegGs, context->EFlags );
        }
        status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
            return STATUS_SUCCESS;
        */

        /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
        if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Eip--;

        /* Ignore vectored handlers for now. The setters for those will print FIXMEs if an application
         * tries to use them.
        if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
            return STATUS_SUCCESS;
        */

        if ((status = call_stack_handlers( rec, context )) != STATUS_UNHANDLED_EXCEPTION)
            return status;
    }

    /* last chance exception */

    /*
    status = send_debug_event( rec, FALSE, context );
    if (status != DBG_CONTINUE)
    {
        if (rec->ExceptionFlags & EH_STACK_INVALID)
            WINE_ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            WINE_ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            WINE_ERR("Unhandled exception code %x flags %x addr %p\n",
                     rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    }
    return STATUS_SUCCESS;
    */
    ntdll_NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
}

NTSTATUS WINAPI ntdll_NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    /* Copypasted from Wine. */
    NTSTATUS status = raise_exception( rec, context, first_chance );
    /* FIXME: It seems that GetCurrentThread() is a function in mingw and not a macro as Wine's code expects. */
    if (status == STATUS_SUCCESS) NtSetContextThread( ((HANDLE)~(ULONG_PTR)1), context );
    return status;
}

#endif /* _WIN64 */

#else

void qemu_NtSetContextThread(struct qemu_syscall *call)
{
    struct qemu_NtSetContextThread *c = (struct qemu_NtSetContextThread *)call;
    WINE_TRACE("\n");

    /* Note that this call returns to us on the host side even if we're setting the current
     * thread's contexts. Our return will return to the qemu main loop, which will resume
     * client side execution wherever the new context points to. So ntdll_NtSetContexThread
     * will not be executed any more. */
    c->super.iret = qemu_ops->qemu_set_context(QEMU_G2H(c->handle), QEMU_G2H(c->context));
}

void qemu_NtRaiseException(struct qemu_syscall *call)
{
    struct qemu_ExceptDebug *c = (struct qemu_ExceptDebug *)call;

    switch (c->num_params)
    {
        case 5:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4, c->p5);
            break;
        case 4:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3, c->p4);
            break;
        case 3:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2, c->p3);
            break;
        case 2:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1, c->p2);
            break;
        case 1:
            WINE_TRACE((const char *)QEMU_G2H(c->string), c->p1);
            break;
        case 0:
            WINE_TRACE((const char *)QEMU_G2H(c->string), 0); /* Hmm, why do I need one arg? */
            break;
    }

    c->super.iret = WINE_TRACE_ON(qemu_ntdll);
}

#endif

#ifdef QEMU_DLL_GUEST

void __fastcall qemu_exception_handler(EXCEPTION_POINTERS *except)
{
    /* Create a copy of the exception info on the stack. */
    EXCEPTION_RECORD rec = *except->ExceptionRecord;
    CONTEXT ctx = *except->ContextRecord;

    ntdll_NtRaiseException(&rec, &ctx, TRUE);
}

#endif

struct qemu_RtlCaptureStackBackTrace
{
    struct qemu_syscall super;
};

#ifdef QEMU_DLL_GUEST

WINBASEAPI USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    struct qemu_RtlCaptureStackBackTrace call;
    call.super.id = QEMU_SYSCALL_ID(CALL_RTLCAPTURESTACKBACKTRACE);

    qemu_syscall(&call.super);

    return call.super.iret;
}

#else

void qemu_RtlCaptureStackBackTrace(struct qemu_syscall *call)
{
    struct qemu_RtlCaptureStackBackTrace *c = (struct qemu_RtlCaptureStackBackTrace *)call;
    WINE_FIXME("Stub!\n");
    c->super.iret = 0;
}

#endif

#ifdef QEMU_DLL_GUEST

void WINAPI ntdll_RtlRaiseStatus(NTSTATUS status);

#if _WIN64

__ASM_GLOBAL_FUNC( RtlRaiseException,
                   __ASM_CFI(".seh_proc	RtlRaiseException\n\t")
                   "movq %rcx,8(%rsp)\n\t"
                   "sub $0x4f8,%rsp\n\t"
                   __ASM_CFI(".seh_stackalloc	0x4f8\n\t")
                   __ASM_CFI(".seh_endprologue\n\t")
                   "leaq 0x20(%rsp),%rcx\n\t"
                   "call " __ASM_NAME("ntdll_RtlCaptureContext") "\n\t"
                   "leaq 0x20(%rsp),%rdx\n\t"   /* context pointer */
                   "leaq 0x500(%rsp),%rax\n\t"  /* orig stack pointer */
                   "movq %rax,0x98(%rdx)\n\t"   /* context->Rsp */
                   "movq (%rax),%rcx\n\t"       /* original first parameter */
                   "movq %rcx,0x80(%rdx)\n\t"   /* context->Rcx */
                   "movq 0x4f8(%rsp),%rax\n\t"  /* return address */
                   "movq %rax,0xf8(%rdx)\n\t"   /* context->Rip */
                   "movq %rax,0x10(%rcx)\n\t"   /* rec->ExceptionAddress */
                   "movl $1,%r8d\n\t"
                   "call " __ASM_NAME("ntdll_NtRaiseException") "\n\t"
                   "movq %rax,%rcx\n\t"
                   "call " __ASM_NAME("ntdll_RtlRaiseStatus") "\n\t" /* does not return */
                   __ASM_CFI(".seh_endproc"));

#else

__ASM_STDCALL_FUNC( RtlRaiseException, 4,
                    "pushl %ebp\n\t"
                    "movl %esp,%ebp\n\t"
                    "leal -0x2cc(%esp),%esp\n\t"  /* sizeof(CONTEXT) */
                    "pushl %esp\n\t"              /* context */
                    "call " __ASM_NAME("ntdll_RtlCaptureContext") __ASM_STDCALL(4) "\n\t"
                    "movl 4(%ebp),%eax\n\t"       /* return address */
                    "movl 8(%ebp),%ecx\n\t"       /* rec */
                    "movl %eax,12(%ecx)\n\t"      /* rec->ExceptionAddress */
                    "leal 12(%ebp),%eax\n\t"
                    "movl %eax,0xc4(%esp)\n\t"    /* context->Esp */
                    "movl %esp,%eax\n\t"
                    "pushl $1\n\t"
                    "pushl %eax\n\t"
                    "pushl %ecx\n\t"
                    "call " __ASM_NAME("ntdll_NtRaiseException") __ASM_STDCALL(12) "\n\t"
                    "pushl %eax\n\t"
                    "call " __ASM_NAME("ntdll_RtlRaiseStatus") __ASM_STDCALL(4) "\n\t"
                    "leave\n\t"
                    "ret $4" )  /* actually never returns */

#endif

#endif

struct qemu__local_unwind
{
    struct qemu_syscall super;
};

#ifdef QEMU_DLL_GUEST

void WINAPI _local_unwind( void *frame, void *target_ip )
{
    struct qemu__local_unwind call;
    call.super.id = QEMU_SYSCALL_ID(CALL__LOCAL_UNWIND);
    qemu_syscall(&call.super);
}

#else

void qemu__local_unwind(struct qemu_syscall *call)
{
    struct qemu__local_unwind *c = (struct qemu__local_unwind *)call;
    WINE_FIXME("Stub!\n");
    c->super.iret = 0;
}

#endif
