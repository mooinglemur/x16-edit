;******************************************************************************
;Copyright 2020-2021, Stefan Jakobsson.
;
;This file is part of X16 Edit.
;
;X16 Edit is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.
;
;X16 Edit is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with X16 Edit.  If not, see <https://www.gnu.org/licenses/>.
;******************************************************************************

.export main_default_entry
.export main_loadfile_entry

;******************************************************************************
;Check build target
.define target_ram 1
.define target_rom 2

.ifndef target_mem
    .error "target_mem not set (1=RAM, 2=ROM)"
.elseif target_mem=1
.elseif target_mem=2
.else
    .error "target_mem invalid value (1=RAM, 2=ROM)"
.endif

;******************************************************************************
;Include global defines
.include "common.inc"
.include "charset.inc"
.include "bridge_macro.inc"

;******************************************************************************
;Description.........: Entry points
jmp main_default_entry
jmp main_loadfile_entry

;******************************************************************************
;Function name.......: main_default_entry
;Purpose.............: Default entry function; starts the editor with an
;                      empty buffer
;Input...............: First RAM bank used by the program in X and last RAM
;                      bank used by the program in Y.
;Returns.............: Nothing
;Error returns.......: None
.proc main_default_entry
    jsr main_init
    bcs exit            ;C=1 => init failed
    jmp main_loop
exit:
    rts
.endproc

;******************************************************************************
;Function name.......: main_loadfile_entry
;Purpose.............: Program entry function
;Input...............: First RAM bank used by the program in X and last RAM
;                      bank used by the program in Y
;Returns.............: Nothing
;Error returns.......: None
.proc main_loadfile_entry
    jsr main_init
    bcs exit            ;C=1 => init failed
    ldx r0
    ldy r0+1
    lda r1
    jsr cmd_file_open
    
    ldx #0
    ldy #2
    jsr cursor_move

    jmp main_loop
exit:
    rts
.endproc

;******************************************************************************
;Function name.......: main_init
;Purpose.............: Initializes the program
;Input...............: First RAM bank used by the program in X and last RAM
;                      bank used by the program in Y. If building the RAM version
;                      the values are ignored and replaced with X=1 and Y=255.
;Returns.............: C=1 if program initialization failed
;Error returns.......: None
.proc main_init
    ;Ensure we are in binary mode
    cld

    ;Save mem_start and mem_top on stack
    .if (::target_mem=target_ram)
        ldx #1
        ldy #255
    .endif

    cpx #0  ;Don't allow bank start = 0, it will mess up the Kernal
    bne :+
    inx

:   phx ;start
    phy ;top

    ;Backup zero page and golden RAM so it can be restored on program exit
    jsr ram_backup

    ;Save ROM bank; used by Kernal bridge code so it knows where to return
    .if (::target_mem=target_rom)
        lda ROM_SEL
        sta rom_bank
    .endif

    ;Set banked RAM start and end
    ply
    plx
    stx mem_start
    sty mem_top

    ;Copy Kernal bridge code to RAM
    .if (::target_mem=target_rom)
        jsr bridge_copy
    .endif

    ;Check if banked RAM start<=top
    ldy mem_start
    cpy mem_top
    bcs err

    ;Set program mode to default
    stz APP_MOD

    ;Initialize base functions
    jsr mem_init
    jsr file_init
    jsr screen_init
    jsr cursor_init
    jsr clipboard_init
    jsr cmd_init
    jsr scancode_init
    jsr progress_init
    
    clc
    rts

    ;Error: mem_top < mem_start - display error message, and restore zero page + golden ram
err:
    bridge_setaddr KERNAL_CHROUT
    ldx #0

:   lda errormsg,x
    beq :+
    bridge_call KERNAL_CHROUT
    inx
    bra :-

:   sec
    jmp ram_restore

errormsg:
    .byt "banked ram allocation error.",0
.endproc

;******************************************************************************
;Function name.......: main_loop
;Purpose.............: Initializes custom interrupt handler and then goes
;                      to the program main loop that just waits for program
;                      to terminate.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc main_loop
    ;Init IRQ
    jsr irq_init

    ;Set program in running state
    stz APP_QUIT

    ;Wait for the program to terminate    
:   lda APP_QUIT        ;0=running, 1=closing down, 2=close now
    cmp #2
    bne :-

    ;Remove custom scancode handler
    jsr scancode_restore
    
    ;Restore zero page and golden RAM from backup taken during program initialization
    jsr ram_restore

exit:
    rts
.endproc

.include "appversion.inc"
.include "screen.inc"
.include "keyboard.inc"
.include "cmd.inc"
.include "cmd_file.inc"
.include "prompt.inc"
.include "irq.inc"
.include "scancode.inc"
.include "cursor.inc"
.include "file.inc"
.include "util.inc"
.include "clipboard.inc"
.include "mem.inc"
.include "ram.inc"
.include "dir.inc"
.include "progress.inc"

.if target_mem=target_rom
    .include "bridge.inc"
.endif