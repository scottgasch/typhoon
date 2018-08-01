%ifdef _X86_
[BITS 32]

;;; 
;;; For some reason MSVC like to put an extra underscore in front of
;;; global identifer names.  If this is gcc, compensate.
;;; 
%ifdef __GNUC__
%define _g_VectorDelta g_VectorDelta
%define _g_PieceData g_PieceData
%endif

[SEGMENT .data]
ParallelScratch:
_ParallelScratch:
    db 43h,30h,50h,56h, 72h,31h,47h,34h, 54h,20h,32h,30h 
    db 30h,36h,20h,53h, 63h,30h,74h,74h, 20h,47h,61h,73h
    db 63h,34h,00h,01h, 22h,15h,48h,35h, 31h,21h,55h,46h

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
        ;; ULONG CDECL
        ;; LastBit(BITBOARD bb)
        ;; 
LastBit:
_LastBit:
        mov ecx, [esp+8]
        mov edx, [esp+4]
        bsr eax, ecx
        jnz .lone
        bsr eax, edx
        jnz .ltwo
        xor eax, eax
        jmp .ldone
.lone:  add eax, 32
.ltwo:  add eax, 1
.ldone: ret

        int 3

[GLOBAL FirstBit]
[GLOBAL _FirstBit]
        ;; 
        ;; ULONG CDECL
        ;; FirstBit(BITBOARD bb)
        ;; 
FirstBit:      
_FirstBit:      
        mov ecx, [esp+8]        ; half of bb
        mov edx, [esp+4]        ; the other half
        bsf eax, edx
        jnz .low
        bsf eax, ecx
        jnz .hi
        xor eax, eax
        ret
.hi:    add eax, 32
.low:   add eax, 1
        ret

        int 3

[GLOBAL CountBits]
[GLOBAL _CountBits]
        ;; 
        ;; ULONG CDECL
        ;; CountBits(BITBOARD bb)
        ;; 
CountBits:     
_CountBits:
        xor eax, eax            ; eax is the count
        mov ecx, [esp+4]        ; half of bb
        test ecx, ecx
        jz .c2                  ; skip first loop?
.c1:    add eax, 1
        mov edx, ecx
        sub edx, 1
        and ecx, edx
        jnz .c1
.c2:    mov ecx, [esp+8]        ; the other half of bb
        test ecx, ecx
        jz .cdone               ; skip second loop?
.c3:    add eax, 1
        mov edx, ecx
        sub edx, 1
        and ecx, edx
        jnz .c3
.cdone: ret

        int 3
%endif

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

%ifndef CROUTINES
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
        push ebp
        mov ebp, esp
        push ebx
        push esi
        push edi
        
        ;; SEE_LIST *pOldList
        ;; COOR c
        ;; int x
        sub esp, 0xC

        mov ecx, [pList]
        mov edx, [pos]

        ;; pOldList = pList;
        mov dword [pOldList], ecx

        ;; pList.uCount = 0;
        mov dword [ecx], 0

        ;; pList = &(pList.data[0]);
        add ecx, 4
        mov edi, ecx

        ;; c = cSquare + iDelta[uColor]
        mov esi, dword [uSide]
        mov ebx, dword [cSquare]
        mov eax, dword [iDelta+esi*4] 
        add ebx, eax

        ;; pPawn = BLACK_PAWN | uColor
        mov ecx, esi
        or ecx, 2

        ;;
        ;; Check the pawns
        ;;
        
        ;; if (!IS_ON_BOARD(c)) goto try other pawn
        test ebx, 0x88
        jnz .try_other_pawn

        ;; p = pos->pSquare[c];
        mov eax, dword [edx+ebx*8+_rgSquare]

        ;; if (p != pPawn) then goto try other pawn
        cmp eax, ecx
        jne .try_other_pawn

        ;; pOldList->uCount = 1
        mov dword [edi-4], 1

        ;; pList->pPiece = p;
        mov dword [edi], ecx

        ;; pList->cLoc = c;
        mov dword [edi+4], ebx

        ;; pList->iVal = VALUE_PAWN;
        mov dword [edi+8], 100

        ;; pList++
        add edi, 12

.try_other_pawn:
        ;; c = c + 2
        add ebx, 2

        ;; if (!IS_ON_BOARD(c)) goto done pawns
        test ebx, 0x88
        jnz .done_pawns

        ;; p = pos->pSquare[c];
        mov eax, dword [edx+ebx*8+_rgSquare]

        ;; if (p != pPawn) then goto done pawns
        cmp eax, ecx
        jne .done_pawns

        ;; pList->pPiece = p;
        mov dword [edi], ecx

        ;; pList->cLoc = c;
        mov dword [edi+4], ebx

        ;; pList->iVal = VALUE_PAWN;
        mov dword [edi+8], 100

        ;; pOldList->uCount++
        ;; pList++
        mov eax, [pOldList]
        add edi, 12
        add dword [eax], 1

.done_pawns:

        ;;
        ;; Do pieces
        ;;
        
        ;; for (x = pos->uNonPawnCount[uSide][0] - 1;
        ;;      x >= 0;
        ;;      x--)
        ;; {
        lea eax, [esi*8]
        mov ecx, dword [edx+eax*4+_uNonPawnCount]
        mov dword [x], ecx

.loop_continue:
        mov ecx, dword [x]
        sub ecx, 1
        cmp ecx, 0
        jl near .done
        mov esi, dword [uSide]
        mov dword [x], ecx

        ;; c = pos->cNonPawns[uSide][x];
        mov eax, esi
        shl eax, 4
        add eax, ecx
        mov eax, dword [edx+eax*4+_cNonPawns]
        mov dword [c], eax

        ;; p = pos->pSquare[c];
        mov ecx, dword [edx+eax*8+_rgSquare]

        ;; iIndex = c - cSquare;
        sub eax, dword [cSquare]

        ;;
        ;; If there is no way for that kind of piece to get to this
        ;; square then keep looking.
        ;;
        ;; if (0 == (g_VectorDeltaTable[iIndex].iVector[iSide] &
        ;;          (1 << (PIECE_PIECE(p) >> 1)))) continue;
        mov ebx, 1
        mov esi, ecx
        shr ecx, 1
        shl ebx, cl
%ifdef OSX
        test byte [_g_NasmVectorDelta+eax*4+512], bl
%else
        test byte [_g_VectorDelta+eax*4+512], bl
%endif
        jz .loop_continue

        ;; if (IS_KNIGHT_OR_KING(p)) goto nothing_blocks
        and ecx, 3
        cmp ecx, 2
        mov ecx, dword [c]
        je .nothing_blocks

        ;;
        ;; Not a knight or king.  Check to see if there is a piece in 
        ;; the path from cSquare to c that blocks the attack.
        ;;
        ;; iDelta = g_VectorDeltaTable[iIndex].iNegDelta;
%ifdef OSX
        movsx ebx, byte [_g_NasmVectorDelta+eax*4+515]
%else
        movsx ebx, byte [_g_VectorDelta+eax*4+515]
%endif

        ;; cBlockIndex = cSquare + iDelta
        mov eax, dword [cSquare]
.block_loop:
        add eax, ebx

        ;; if (cBlockIndex != c)
        ;; {
        cmp eax, ecx
        je .nothing_blocks

        ;;     if (IS_EMPTY(pos->pSquare[cBlockIndex]))
        ;;     {
        cmp dword [edx+eax*8], 0
        jne .loop_continue

        ;; goto cond
        jmp .block_loop

.nothing_blocks:
        ;; pList->pPiece[pList->iCount] = p;
        mov dword [edi], esi

        ;; pList->cLoc[pList->iCount] = c;
        mov dword [edi+4], ecx

        ;; pList->iVal[pList->iCount] = PIECE_VALUE(p)
        mov ecx, esi
        shr ecx, 1
        shl ecx, 4
%ifdef OSX
        mov ebx, dword [_g_NasmPieceData+ecx]
%else
        mov ebx, dword [_g_PieceData+ecx]
%endif
        mov dword [edi+8], ebx

        ;; uCount++
        mov eax, dword [pOldList]
        add edi, 12
        add dword [eax], 1
        jmp .loop_continue

.done:  add esp, 0xC
        pop edi                         ; restore saved registers
        pop esi
        pop ebx
        leave
        ret
%endif ; CROUTINES
%endif ; X86
