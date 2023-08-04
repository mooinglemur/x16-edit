.proc selection_activate
    lda #1
    sta selection_active
    rts
.endproc

.proc selection_collapse
    stz selection_active
    rts
.endproc

;******************************************************************************
;Function name.......: selection_char_count
;Purpose.............: Calculates the number of visible chars on a line that
;                      is to be printed to the screen; firstly chars visible
;                      before the selection starts, and secondly visible chars
;                      that are selected. This function is intended to be
;                      used by the screen print line and refresh functions
;                      to decide what colors to output
;Input...............: screen_print_line (24 bits)
;                      screen_print_col (24 bits)
;                      selection_start_line (24 bits)
;                      selection_start_col (24 bits)
;                      selection_end_line (24 bits)
;                      selection_end_col (24 bits)
;Returns.............: X: Number of unselected chars at start of line
;                      Y: Number of selected chars
;Error returns.......: None
.export selection_char_count
.export screen_print_line, screen_print_col
.export selection_start_line, selection_end_line, selection_start_col, selection_end_col
.export screen_width

.proc selection_char_count
    ;Check if selection is active
    lda selection_active
    bne :+
    jmp no_selection
    
    ;Init
:   stz flags

    ;PART I: Check if the line to be printed to screen is within the line where the
    ;        selection starts and ends
    ;------------------------------------------------------------------------------

    ;Calculate Selection End Line - Screen Print Line
    ldx #0
    sec
    lda selection_end_line
    sbc screen_print_line
    beq :+
    tax
:   lda selection_end_line+1
    sbc screen_print_line+1
    beq :+
    tax
:   lda selection_end_line+2
    sbc screen_print_line+2
    beq :+
    tax
:   bcs :+                   ;Branch if Selection End Line >= Screen Print Line
    jmp no_selection         ;Selection End Line < Screen Print Line => No selection on this line
:   cpx #1                   ;Clear flag if Screen Print Line is at Selection End Line
    rol flags

    ;Calculate Screen Print Line - Selection Start Line
    ldx #0
    sec
    lda screen_print_line
    sbc selection_start_line
    beq :+
    tax
:   lda screen_print_line+1
    sbc selection_start_line+1
    beq :+
    tax
:   lda screen_print_line+2
    sbc selection_start_line+2
    beq :+
    tax
:   bcs :+                  ;Branch if Screen Print Line >= Selection Start Line
    jmp no_selection        ;Screen Print Line < Selection Start Line => No selection on this line
:   cpx #1                  ;Clear flag if Screen Print Line is at Selection Start Line
    rol flags               ;Bit 0 = clear if Screen Print Line == Selection Start Line
                            ;Bit 1 = clear if Screen Print Line == Selection End Line

    ;Screen print line is within a selection, but not on the first or last line of it => The whole line is selected
    lda flags
    cmp #%00000011
    bne left_distance

    ldx #0
    ldy screen_width
    rts

    ;PART II: Calculate number of visible chars on the line to be printed that are selected and 
    ;         not selected
    ;
    ;         Pseudo code:
    ;
    ;         dleft = selection_start_col - print_col
    ;         if dleft < 0:
    ;           dleft = 0
    ;           dmid = selection_end_col - print_col
    ;         elif dleft > screen_width:
    ;           dleft = screen_width
    ;           dmid = 0
    ;         else:
    ;           dmid = selection_end_col - selection_start_col
    ;
    ;         #Adjust values for selection start and end lines if an multi-line selection
    ;         if flags & %00000010: #Line to be printed on selection start line, but not selection end line
    ;           dmid = screen_width - dleft
    ;         elif flags & %00000001 #Line to be printed on selection end line, but not selection start line
    ;           dmid = dleft + dmid
    ;           dleft = 0
    ;         
    ;         #Normalize dmid to range 0..screen_width 
    ;         if dleft + dmid > screen_width:
    ;           dmid = screen_width - dleft
    ;
    ;         Where:
    ;           dleft   = number of visible chars on the line to be printed before the selection
    ;           dmid    = number of visible chars on the line to be printed that are selected

left_distance:
    sec
    lda selection_start_col
    sbc screen_print_col
    sta dleft
    lda selection_start_col+1
    sbc screen_print_col+1
    sta dtmp
    lda selection_start_col+2
    sta dtmp+1
    bcc noleft

    ;Normalize output to range 0..screen_width
    ora dtmp
    bne :+
    lda dleft
    cmp screen_width
    bcc mid_distance

:   ldx screen_width
    ldy #0
    bra multiline_adjust

noleft:
    stz dleft

    sec
    lda selection_end_col
    sbc screen_print_col
    sta dmid
    lda selection_end_col+1
    sbc screen_print_col+1
    sta dtmp
    lda selection_end_col+2
    sbc screen_print_col+2
    bcc no_selection

    ;Normalize output to range 0..screen_width
    inc dmid
    ora dtmp
    bne :+
    lda screen_width
    cmp dmid
    bcs multiline_adjust

:   ldx #0
    ldy screen_width
    bra multiline_adjust

mid_distance:
    sec
    lda selection_end_col
    sbc selection_start_col
    sta dmid
    lda selection_end_col+1
    sbc selection_start_col+1
    sta dtmp
    lda selection_end_col+2
    sbc selection_start_col+2
    
    ;Normalize output to range 0..screen_width
    inc dmid
    ora dtmp
    bne :+
    clc
    lda dmid
    adc dleft
    bcs :+
    cmp screen_width
    bcc multiline_adjust

:   sec
    lda screen_width
    sbc dleft
    sta dmid

multiline_adjust:
    lda flags
    cmp #%00000010           ;On first line, but not last line of selection
    bne :+
    
    sec
    lda screen_width
    sbc dleft
    sta dmid

:   cmp #%00000001           ;On last line, but not first line of selection
    bne :+

    clc
    lda dleft
    adc dmid
    sta dmid
    stz dleft

:   ldx dleft
    ldy dmid
    rts


no_selection:
    ldx screen_width
    ldy #0
    rts

flags = tempvars
dleft = tempvars+1
dmid = tempvars+2
dtmp = tempvars+3
.endproc

.segment "VARS"
    selection_active:       .res 1
    selection_start_line:    .res 3
    selection_start_col:    .res 3
    selection_end_line:      .res 3
    selection_end_col:      .res 3
.CODE