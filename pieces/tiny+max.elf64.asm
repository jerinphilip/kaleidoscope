;; tiny.asm: Copyright (C) 2011 Brian Raiter <breadbox@muppetlabs.com>
;; Licensed under the terms of the GNU General Public License, either
;; version 2 or (at your option) any later version.
;;
;; To build:
;;	nasm -f bin -o tiny tiny.asm && chmod +x tiny

BITS 64

      org   0x2ABF00000000
ehdr:                              ; Elf64_Ehdr
      db   0x7F, "ELF", 2, 1, 1, 1 ; e_ident
      dq   0
      dw   2                       ; e_type
      dw   62                      ; e_machine
      dd   1                       ; e_version
      dq   _start                  ; e_entry
      dq   phdr - $$               ; e_phoff
      dq   0                       ; e_shoff
      dd   0                       ; e_flags
      dw   ehdrsz                  ; e_ehsize
      dw   phdrsz                  ; e_phentsize
      dw   1                       ; e_phnum
      dw   0                       ; e_shentsize
      dw   0                       ; e_shnum
      dw   0                       ; e_shstrndx
ehdrsz      equ   $ - ehdr

phdr:                              ; Elf64_Phdr
      dd   1                       ; p_type
      dd   5                       ; p_flags
      dq   0                       ; p_offset
      dq   $$                      ; p_vaddr
      dq   $$                      ; p_paddr
      dq   filesz                  ; p_filesz
      dq   filesz                  ; p_memsz
      dq   0x1000                  ; p_align

phdrsz      equ   $ - phdr


_start:
      mov   edi, 42                ; arg1 = 41
      mov   esi, 41                ; arg2 = 42
      call _Z3maxii                ; call max(41, 42)
      mov   edi, 0                 ; (optional): edi to 0 for blinkenlights visibility.
      mov   edi, eax               ; register edi = return-value
      mov   eax, 60                ; register eax = syscall number
      syscall                      ; exit(register edi)

_Z3maxii:
      cmp     edi, esi             ; cmp arg2, arg1 
      mov     eax, esi             ; assume result is arg2
      cmovge  eax, edi             ; if greater result is arg1
      ret

filesz      equ   $ - $$
