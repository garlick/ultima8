	TITLE	"Alcor 1.2 for PIC16F84A - M. Covington 1999"

;  Alcor telescope drive corrector, version 1.2, 1999 May 22.
;
;	For PIC16F84 with 4.000-MHz crystal.
;	Should port to lower PICs with 13 i/o pins.
; 
;	(Version 1.0 was for Philips 87C750.  Versions 1.1 and 1.1a
;       had a less accurate lunar rate.)
;
;	Pin assignments:
;
;	  Inputs, all active low, with weak pullups:
;
;   	     Buttons:
;		B0	_NORTH		Ground this pin to slew northward
;		B1	_SOUTH		Ground to slew southward
;		B2	_EAST		Ground to slew eastward (slow down)
;		B3	_WEST		Ground to slew westward (speed up)
;	     Jumpers:
;		B4	_LUNAR		Ground to select lunar rate
;		B5	_50HZ		Ground to select 50 Hz
;		B6	_SWAPNS		Ground to swap _NORTH and _SOUTH
;		B7	_SWAPEW		Ground to swap _EAST and _WEST	
;
;	   Outputs:
;
;		A0	GONORTH		High when dec motor shd go north
;		A1	GOSOUTH		High when dec motor shd go south
;		A2	PHASE1		Phase 1 of 2-phase drive
;		A3	PHASE2		Phase 2 of 2-phase drive
;		A4	SQWAVE		Square wave out (REQUIRES PULL-UP)
;	
;
; Remarks on program logic:
;
;	The basic task of this program is to produce output waveforms
;	of the correct frequency at all times, depending on buttons.
;	Frequency changes are gradual.  
;
;	The program has a table of 20 frequencies (indexed 0..9 for 50Hz
;	and 0..9 for 60Hz), most of which are used only during transi-
;	tions:
;		Frequency 0 = west slewing
;		Frequency 4 = sidereal rate
;		Frequency 5 = lunar rate
;		Frequency 9 = east slewing
;
;	ACTFRQ is the frequency currently in use, and DESFRQ is the
;	frequency that the user is requesting.  ACTFRQ is incremented
;	or decremented once per AC cycle to make it match DESFRQ.
;
;	During the delay loop, there is a 20-way branch that selects
;	the right length of delay for the variable part of the AC cycle.
;	There are 10 speeds each for 50-Hz and 60-Hz base frequencies.
;
;
; Remark on programming technique:
;
;	Conditionals within timing loops are arranged so that they
;	take equal amounts of time on either branch.  On the PIC,
;	this is done as follows:
;	
;		btfsc	addr,bit	;; if bit is 0 
;		goto	L1		;;
;		nop			;; then
;		-------------------
;		-- block of code --
;		-------------------
;		goto	L2		;; else
;	L1				;; 
;		-------------------
;		-- block of code --
;		-------------------
;		goto	L2		;; end if
;	L2				;;
;
;	This takes 5 cycles on either branch, plus the time
;	taken by the appropriate block of code.
;
;	A much simpler approach, if only a single instruction
;	is involved, is something like this:
;
;		btfsc	reg,bit		;; if bit is 1
;		xorwf	reg		;; do this
;
;	If the skip is taken, a nop is executed in place of xorwf.
;
;
; Frequencies and delays
;
;	Drive rate		60 Hz motor	50 Hz motor
;
;	Sidereal		60.164 Hz	50.137 Hz
;	Lunar			58.696 Hz	48.912 Hz   ; updated ver. 1.2
;        (Average topocentric, moon high in sky, temperate latitudes)
;	East slewing (4"/sec)	44.164 Hz	36.804 Hz
;	West slewing (4"/sec)	76.164 Hz	63.470 Hz	
;
; 	Caution!  Because of the use of integer counts in the
;	program, and because of crystal tolerances, these
;	frequencies are accurate only to 0.01% (100 ppm).
;	For much greater accuracy, trim the crystal frequency
;	with a variable capacitor.
;
;	The timing loop has a fixed part and a variable part.
;
;	Fixed part of loop	4156		4987 CPU cycles
;
;	Variable part of loop:
;
;		freq 0		2410		2981
;		freq 1		2761		3313
;		freq 2		3164		3797
;		freq 3		3626		4351
;		freq 4 (sid.)	4155		4986
;		freq 5 (lunar)	4363		5236  ; updated ver. 1.2
;		freq 6		5032		6038
;		freq 7		5661		6793
;		freq 8		6369		7643
;		freq 9		7166		8592
;
;	Main loop takes 64 cycles in addition to the delay loops,
;	so we subtract 32 cy. from the fixed part and 32 from var. part.
;


;
; CPU CONFIGURATION
;

	processor 16f84a
	include	  <p16f84a.inc>

	__config  _XT_OSC & _WDT_OFF & _PWRTE_ON

	; Oscillator type XT
	; Watchdog timer OFF
	; Power-up timer ON
	;
	; (In MPLAB, be sure to set these from menus.)

        ; Store identifying data in data EEPROM

;	ORG	2100H
;	DE	"Alcor 1.2 Copyright 1997 M. Covington"

;
; DATA MEMORY
;

DESFRQ	equ	0x1F	; desired frequency setting
ACTFRQ	equ	0x1E	; frequency setting currently in use
BUTTONS	equ	0x1D	; holds button input bits
PHFLAG	equ	0x1C	; flag for distinguishing the 2 phases

R1	equ	0x1B	; used by delay loops
R2	equ	0x1A	; used by delay loops
R3      equ     0x19    ; used by delay loops
R4      equ     0x18    ; used by delay loops

_NORTH	equ	0	; Bit numbers for BUTTONS and PORTB
_SOUTH	equ	1
_EAST	equ	2
_WEST	equ	3
_LUNAR	equ	4
_50HZ 	equ	5
_SWAPNS	equ	6
_SWAPEW	equ	7

GONORTH	equ	0	; Bit numbers for PORTA
GOSOUTH	equ	1
PHASE1	equ	2
PHASE2	equ	3
SQWAVE	equ	4


;
; MAIN PROGRAM
;

	org	0

START

;	Initialize variables

	movlw	4
	movwf	DESFRQ		; DESFRQ := 4
	movwf	ACTFRQ		; ACTFRQ := 4

	clrf	PHFLAG		; PHFLAG := b'00000000'

;	Initialize port A as outputs

	movlw	b'00000000'
	tris	PORTA		; deprecated instruction
	clrf	PORTA		; take all outputs low

;	Initialize port B as input

	movlw 	b'11111111'
	tris	PORTB		; deprecated instruction

;	Enable weak pull-ups on port B

	movlw	b'01111111'
	option			; deprecated instruction


MAIN

;	Copy _NORTH, _SOUTH, _EAST, _WEST from PORTB into BUTTONS

	movfw	PORTB
	andlw	b'00001111'
	movwf	BUTTONS

;	If _SWAPNS is active (low), toggle both _NORTH and _SOUTH

	movlw	b'00000011'
	btfss	PORTB,_SWAPNS
	xorwf	BUTTONS,F

;	If _NORTH and _SOUTH are both 0, set them both to 1
;
;	(This handles two situations: user pressing both buttons,
;	or user pressing neither button and swap requested.)

	movfw	BUTTONS
	andlw	b'00000011'	; isolate bottom two bits

	btfsc	STATUS,Z	; if result was zero
	movlw	b'00000011'	; accumulator = 00000011

	iorwf	BUTTONS,F	; put back in BUTTONS
	

;	If _SWAPEW is active (low), toggle both _EAST and _WEST

	movlw	b'00001100'
	btfss	PORTB,_SWAPEW
	xorwf	BUTTONS,F


;	If _EAST and _WEST are both 0, set them both to 1

	movfw	BUTTONS
	andlw	b'00001100'	; isolate the two bits

	btfsc	STATUS,Z	; if result was 0
	movlw	b'00001100'	; accumulator = 00001100

	iorwf	BUTTONS,F	; put back in BUTTONS
	

;	Set declination outputs, avoiding glitches

	btfsc	BUTTONS,_NORTH	; if BUTTONS._NORTH = 1
	bcf	PORTA,GONORTH	;  then GONORTH := 0

	btfss	BUTTONS,_NORTH	; if BUTTONS._NORTH = 0
	bsf	PORTA,GONORTH	;  then GONORTH := 1
	
	btfsc	BUTTONS,_SOUTH	; if BUTTONS._SOUTH = 1
	bcf	PORTA,GOSOUTH	;  then GOSOUTH := 0

	btfss	BUTTONS,_SOUTH	; if BUTTONS._SOUTH = 0
	bsf	PORTA,GOSOUTH	;  then GOSOUTH := 1


;	Select desired frequency

	movlw	4		; default is sidereal rate, 4

	btfss	PORTB,_LUNAR	; if lunar rate selected, use 5
	movlw	5		

	btfss	BUTTONS,_EAST	; if east slewing, use 9
	movlw	9

	btfss	BUTTONS,_WEST	; if west slewing, use 0
	movlw	0

	movwf	DESFRQ		; DESFRQ := what was just chosen


;	Adjust actual frequency for this cycle

	; Comparisons rely on the fact that SUBWF
	; *clears* the Carry flag iff the result is negative,
	; and sets it otherwise.

	; If ACTFRQ < DESFRQ, increment ACTFRQ

	movfw	DESFRQ		
	subwf	ACTFRQ,W	; w := ACTFRQ - DESFRQ
	btfss	STATUS,C	; if negative result then
	incf	ACTFRQ,F	; increment ACTFRQ

	; If DESFRQ < ACTFRQ, decrement ACTFRQ

	movfw	ACTFRQ
	subwf	DESFRQ,W	; w := DESFRQ - ACTFRQ
	btfss	STATUS,C	; if negative result then
	decf	ACTFRQ,F	; decrement ACTFRQ


;	Perform fixed-length part of cycle

	; toggle SQWAVE

	movlw	B'00010000'	; pick out SQWAVE
	xorwf	PORTA,F		; toggle it

	; raise PHASE1 or PHASE2 as appropriate

	comf	PHFLAG,F	; toggle PHFLAG

	btfss	PHFLAG,1	; if PHFLAG.1
	bsf	PORTA,PHASE1	;  set PHASE1

	btfsc	PHFLAG,1	; if PHFLAG.1
	bsf	PORTA,PHASE2	;  set PHASE2

	; delay code

;;;;;;
; Code generated by PICDELAY v5 10-16-1997 19:48:11
; Delay loop,  4124  cycles
;
        MOVLW   0x56
        MOVWF   R1
        MOVLW   0x6
        MOVWF   R2
DELAY1: 
        DECFSZ  R1,F
        GOTO    DELAY1
        DECFSZ  R2,F
        GOTO    DELAY1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
	NOP
;
;;;;;;

	; extra delay if 50 Hz

	btfsc	PORTB,_50HZ	; if _50HZ low
	goto	L10		; then
	nop

;;;;;;
; Code generated by PICDELAY v5 10-16-1997 19:48:11
; Delay loop,  831  cycles
;
        MOVLW   0x10
        MOVWF   R1
        MOVLW   0x2
        MOVWF   R2
DELAY2: 
        DECFSZ  R1,F
        GOTO    DELAY2
        DECFSZ  R2,F
        GOTO    DELAY2
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

L10				; end if

	; lower PHASE1 and PHASE2

	bcf	PORTA,PHASE1
	bcf	PORTA,PHASE2

;	Depending on actfrq, perform appropriate delay for
;	variable-length part of cycle

	; Jump table has 20 entries,
	; corresponding to freqs 0-9 for 60 Hz motor
	; and then freqs 0-9 for 50 Hz motor.

	movfw	ACTFRQ		; w := ACTFRQ

	btfss	PORTB,_50HZ	; if 50 Hz selected then
	addlw	D'10'		; w := w + 10

	addwf	PCL,F		; Add w to program counter.
				; WARNING!  We must be
				; entirely within the
				; first 256 words of program
				; memory to avoid a page boundary.

	goto	d60_0		; The infamous 20-way branch.
	goto	d60_1
	goto	d60_2
	goto	d60_3
	goto	d60_4
	goto	d60_5
	goto	d60_6
	goto	d60_7
	goto	d60_8
	goto	d60_9
	goto	d50_0
	goto	d50_1
	goto	d50_2
	goto	d50_3
	goto	d50_4
	goto	d50_5
	goto	d50_6
	goto	d50_7
	goto	d50_8
	goto	d50_9

	; It's OK for the GOTOs to jump across a page boundary.
	; Accordingly, the following code can fall anywhere.
d60_0
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  2378  cycles
;
;
        MOVLW   0x12
        MOVWF   R1
        MOVLW   0x4
        MOVWF   R2
DELAY1001: 
        DECFSZ  R1,F
        GOTO    DELAY1001
        DECFSZ  R2,F
        GOTO    DELAY1001
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        NOP
;
;;;;;;

        GOTO MAIN

d60_1
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  2729  cycles
;
;
        MOVLW   0x87
        MOVWF   R1
        MOVLW   0x4
        MOVWF   R2
DELAY1002: 
        DECFSZ  R1,F
        GOTO    DELAY1002
        DECFSZ  R2,F
        GOTO    DELAY1002
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        NOP
;
;;;;;;

        GOTO MAIN

d60_2
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  3132  cycles
;
;
        MOVLW   0xD
        MOVWF   R1
        MOVLW   0x5
        MOVWF   R2
DELAY1003: 
        DECFSZ  R1,F
        GOTO    DELAY1003
        DECFSZ  R2,F
        GOTO    DELAY1003
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d60_3
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  3594  cycles
;
;
        MOVLW   0xA7
        MOVWF   R1
        MOVLW   0x5
        MOVWF   R2
DELAY1004: 
        DECFSZ  R1,F
        GOTO    DELAY1004
        DECFSZ  R2,F
        GOTO    DELAY1004
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d60_4
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  4123  cycles
;
;
        MOVLW   0x56
        MOVWF   R1
        MOVLW   0x6
        MOVWF   R2
DELAY1005: 
        DECFSZ  R1,F
        GOTO    DELAY1005
        DECFSZ  R2,F
        GOTO    DELAY1005
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d60_5

;;;;;;;
;; Code generated by PICDELAY v5 10-16-1997 20:16:38
;; Delay loop,  4440  cycles
;;
;;
;        MOVLW   0xC0
;        MOVWF   R1
;        MOVLW   0x6
;        MOVWF   R2
;DELAY1006: 
;        DECFSZ  R1,F
;        GOTO    DELAY1006
;        DECFSZ  R2,F
;        GOTO    DELAY1006
;        GOTO    $+1
;        GOTO    $+1
;        GOTO    $+1
;;;;     GOTO    $+1   ; eliminated 1999 Jan 22
;        NOP
;;
;;;;;;;

;;;;;;
; Code generated by PICDELAY v5 05-22-1999 14:15:36
; Delay loop,  4331  cycles
;

        MOVLW   0x9C
        MOVWF   R1
        MOVLW   0x6
        MOVWF   R2
        MOVLW   0x1
        MOVWF   R3
        MOVLW   0x1
        MOVWF   R4
DELAY1006: 
        DECFSZ  R1,F
        GOTO    DELAY1006
        DECFSZ  R2,F
        GOTO    DELAY1006
        DECFSZ  R3,F
        GOTO    DELAY1006
        DECFSZ  R4,F
        GOTO    DELAY1006
;
;;;;;;

        GOTO MAIN




d60_6
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  5000  cycles
;
;
        MOVLW   0x7A
        MOVWF   R1
        MOVLW   0x7
        MOVWF   R2
DELAY1007: 
        DECFSZ  R1,F
        GOTO    DELAY1007
        DECFSZ  R2,F
        GOTO    DELAY1007
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        NOP
;
;;;;;;

        GOTO MAIN

d60_7
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  5629  cycles
;
;
        MOVLW   0x4B
        MOVWF   R1
        MOVLW   0x8
        MOVWF   R2
DELAY1008: 
        DECFSZ  R1,F
        GOTO    DELAY1008
        DECFSZ  R2,F
        GOTO    DELAY1008
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        NOP
;
;;;;;;

        GOTO MAIN

d60_8
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  6337  cycles
;
;
        MOVLW   0x36
        MOVWF   R1
        MOVLW   0x9
        MOVWF   R2
DELAY1009: 
        DECFSZ  R1,F
        GOTO    DELAY1009
        DECFSZ  R2,F
        GOTO    DELAY1009
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d60_9
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  7134  cycles
;
;
        MOVLW   0x3F
        MOVWF   R1
        MOVLW   0xA
        MOVWF   R2
DELAY1010: 
        DECFSZ  R1,F
        GOTO    DELAY1010
        DECFSZ  R2,F
        GOTO    DELAY1010
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_0
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  2949  cycles
;
;
        MOVLW   0xD0
        MOVWF   R1
        MOVLW   0x4
        MOVWF   R2
DELAY1011: 
        DECFSZ  R1,F
        GOTO    DELAY1011
        DECFSZ  R2,F
        GOTO    DELAY1011
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_1
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  3281  cycles
;
;
        MOVLW   0x3E
        MOVWF   R1
        MOVLW   0x5
        MOVWF   R2
DELAY1012: 
        DECFSZ  R1,F
        GOTO    DELAY1012
        DECFSZ  R2,F
        GOTO    DELAY1012
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_2
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  3765  cycles
;
;
        MOVLW   0xE0
        MOVWF   R1
        MOVLW   0x5
        MOVWF   R2
DELAY1013: 
        DECFSZ  R1,F
        GOTO    DELAY1013
        DECFSZ  R2,F
        GOTO    DELAY1013
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_3
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  4319  cycles
;
;
        MOVLW   0x98
        MOVWF   R1
        MOVLW   0x6
        MOVWF   R2
DELAY1014: 
        DECFSZ  R1,F
        GOTO    DELAY1014
        DECFSZ  R2,F
        GOTO    DELAY1014
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_4
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  4954  cycles
;
;
        MOVLW   0x6B
        MOVWF   R1
        MOVLW   0x7
        MOVWF   R2
DELAY1015: 
        DECFSZ  R1,F
        GOTO    DELAY1015
        DECFSZ  R2,F
        GOTO    DELAY1015
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_5
;;;;;;;
;; Code generated by PICDELAY v5 10-16-1997 20:16:38
;; Delay loop,  5334  cycles
;;
;;
;        MOVLW   0xE9
;        MOVWF   R1
;        MOVLW   0x7
;        MOVWF   R2
;DELAY1016: 
;        DECFSZ  R1,F
;        GOTO    DELAY1016
;        DECFSZ  R2,F
;        GOTO    DELAY1016
;        GOTO    $+1
;        GOTO    $+1
;        GOTO    $+1
;        GOTO    $+1
;;;;     GOTO    $+1   ; eliminated 1999 Jan 22
;;
;;;;;;;

;;;;;;
; Code generated by PICDELAY v5 05-22-1999 14:15:54
; Delay loop,  5204  cycles
;

        MOVLW   0xBE
        MOVWF   R1
        MOVLW   0x7
        MOVWF   R2
        MOVLW   0x1
        MOVWF   R3
        MOVLW   0x1
        MOVWF   R4
DELAY1016: 
        DECFSZ  R1,F
        GOTO    DELAY1016
        DECFSZ  R2,F
        GOTO    DELAY1016
        DECFSZ  R3,F
        GOTO    DELAY1016
        DECFSZ  R4,F
        GOTO    DELAY1016
        NOP
;
;;;;;;


        GOTO MAIN

d50_6
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  6006  cycles
;
;
        MOVLW   0xC9
        MOVWF   R1
        MOVLW   0x8
        MOVWF   R2
DELAY1017: 
        DECFSZ  R1,F
        GOTO    DELAY1017
        DECFSZ  R2,F
        GOTO    DELAY1017
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_7
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  6761  cycles
;
;
        MOVLW   0xC4
        MOVWF   R1
        MOVLW   0x9
        MOVWF   R2
DELAY1018: 
        DECFSZ  R1,F
        GOTO    DELAY1018
        DECFSZ  R2,F
        GOTO    DELAY1018
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_8
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  7611  cycles
;
;
        MOVLW   0xDE
        MOVWF   R1
        MOVLW   0xA
        MOVWF   R2
DELAY1019: 
        DECFSZ  R1,F
        GOTO    DELAY1019
        DECFSZ  R2,F
        GOTO    DELAY1019
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN

d50_9
;;;;;;
; Code generated by PICDELAY v5 10-16-1997 20:16:38
; Delay loop,  8560  cycles
;
;
        MOVLW   0x19
        MOVWF   R1
        MOVLW   0xC
        MOVWF   R2
DELAY1020: 
        DECFSZ  R1,F
        GOTO    DELAY1020
        DECFSZ  R2,F
        GOTO    DELAY1020
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
        GOTO    $+1
;
;;;;;;

        GOTO MAIN


	END

