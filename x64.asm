%ifdef _X64_
[BITS 64]

[SEGMENT .data]

[EXTERN _g_VectorDelta]
[EXTERN _g_PieceData]

%ifdef OSX
;;; Note: The reason for this OSX stuff is that there is a bug in the mac
;;; version of nasm.  See comments in data.c for more details.
       
[GLOBAL g_NasmVectorDelta]
[GLOBAL _g_NasmVectorDelta]
g_NasmVectorDelta:
_g_NasmVectorDelta: 
    alignb 32, db 0
    times 256 * 4 db 0

[GLOBAL g_NasmPieceData]
[GLOBAL _g_NasmPieceData]
g_NasmPieceData:
_g_NasmPieceData:               
    alignb 32, db 0
    times 4 * 4 * 8 db 0
%else
[EXTERN _g_VectorDelta]
[EXTERN _g_PieceData]
%endif
        
[SEGMENT .text]

%ifndef CROUTINES        
[GLOBAL LastBit]
[GLOBAL _LastBit]
        ;; 
        ;; ULONGLONG CDECL
        ;; LastBit(BITBOARD bb)
        ;; 
LastBit:
_LastBit:
        int 3
        bsr rax, rcx
        jnz .found
        xor rax, rax
        ret
.found: add rax, 1
        ret
        int 3

[GLOBAL FirstBit]
[GLOBAL _FirstBit]
        ;; 
        ;; ULONGLONG CDECL
        ;; FirstBit(BITBOARD bb)
        ;; 
FirstBit:
_FirstBit:
        ;movq rcx, [rsp+8]
        bsf rax, rcx
        jnz .found
        xor rax, rax
        ret
.found: add rax, 1
        ret
        int 3

[GLOBAL CountBits]
[GLOBAL _CountBits]
        ;; 
        ;; ULONGLONG CDECL
        ;; CountBits(BITBOARD bb)
        ;; 
CountBits:     
_CountBits:
        xor rax, rax
        mov rcx, [esp+8]
        test rcx, rcx
        jz .done
.again: add rax, 1
        mov rdx, rcx
        sub rdx, 1
        and rcx, rdx
        jnz .again
.done:  ret
        int 3

[GLOBAL GetAttacks]
[GLOBAL _GetAttacks]

        db 43h,30h,50h,56h,72h,31h,47h,34h,54h,20h,32h,30h,30h
        db 36h,20h,53h,63h,30h,74h,74h,20h,47h,61h,73h,63h,34h
        
iDelta  dd -17
        dd +15

%define uSide    ebp+0x14
%define cSquare  ebp+0x10
%define pos      ebp+0xC
%define pList    ebp+8
;;      retaddr  ebp+4
;;      old ebp  ebp
;;      old ebx  ebp-4
;;      old esi  ebp-8
;;      old edi  ebp-0xC
%define pOldList ebp-0x10
%define c        ebp-0x14
%define x        ebp-0x18

%define _cNonPawns     0x478
%define _uNonPawnCount 0x500
%define _rgSquare      0x0
                        
        ;; 
        ;; void CDECL
        ;; GetAttacks(SEE_LIST *pList,  ; ebp + 8
        ;;            POSITION *pos,    ; ebp + 0xC
        ;;            COOR cSquare,     ; ebp + 0x10
        ;;            ULONG uSide)      ; ebp + 0x14
        ;; 
GetAttacks:
_GetAttacks:
        int 3
%endif ; !CROUTINES

[GLOBAL LockCompareExchange]
[GLOBAL _LockCompareExchange]
%define uComp esp+0xC
%define uExch esp+8
%define pDest esp+4
        ;; 
        ;; ULONG CDECL
        ;; LockCompareExchange(void *dest, ; esp + 4
        ;;                     ULONG exch, ; esp + 8
        ;;                     ULONG comp) ; esp + C
        ;; 
LockCompareExchange:
_LockCompareExchange:
        mov ecx, [pDest]
        mov edx, [uExch]
        mov eax, [uComp]
        lock cmpxchg dword [ecx], edx
        ret
        int 3

[GLOBAL LockIncrement]
[GLOBAL _LockIncrement]
        ;;
        ;; ULONG CDECL
        ;; LockIncrement(ULONG *pDest)
        ;; 
LockIncrement:
_LockIncrement:
        mov ecx, [pDest]
        mov eax, 1
        lock xadd [ecx], eax
        add eax, 1
        ret
        int 3

[GLOBAL LockDecrement]
[GLOBAL _LockDecrement]
        ;;
        ;; ULONG CDECL
        ;; LockDecrement(ULONG *pDest)
        ;; 
LockDecrement:
_LockDecrement:
        mov ecx, [pDest]
        mov eax, -1
        lock xadd [ecx], eax
        add eax, -1
        ret
        int 3

%endif ; X64
