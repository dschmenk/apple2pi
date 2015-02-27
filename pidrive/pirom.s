;****************************************
;*
;* PIDRIVE (PI DRIVE) ROM
;*
;****************************************

PIROM	= 1
.DEFINE	SIG	"PI"
	.INCLUDE "romincs.s"
	
;****************************************
;*
;*      PER-SLOT ROM @ $Cx00
;*
;****************************************
	ORG	$C000

.SCOPE
BANKSEL	EQU	$C08F
	.INCLUDE "cxrom.s"	; DUMMY SLOT - NEVER USED
.ENDSCOPE
.SCOPE
BANKSEL	EQU	$C09F
	.INCLUDE "cxrom.s"
.ENDSCOPE
.SCOPE
BANKSEL	EQU	$C0AF
	.INCLUDE "cxrom.s"
.ENDSCOPE
.SCOPE
BANKSEL	EQU	$C0BF
	.INCLUDE "cxrom.s"
.ENDSCOPE
.SCOPE
BANKSEL	EQU	$C0CF
	.INCLUDE "cxrom.s"
.ENDSCOPE
.SCOPE
BANKSEL	EQU	$C0DF
	.INCLUDE "cxrom.s"
.ENDSCOPE
.SCOPE
BANKSEL	EQU	$C0EF
	.INCLUDE "cxrom.s"
.ENDSCOPE
.SCOPE
BANKSEL	EQU	$C0FF
	.INCLUDE "cxrom.s"
.ENDSCOPE
	
;****************************************
;*
;*      OPTION ROM SPACE @ $C800
;*
;****************************************
	.ASSERT	* = $C800, error, "Slot ROM not page aligned"
;	ORG	$C800

	.INCLUDE "c8rom.s"
;*
;* FILL REMAINING ROM WITH 0'S
;*
	.REPEAT $D000-*
	DB	$00
	.ENDREP
	.ASSERT	* = $D000, error, "Code not page size"
