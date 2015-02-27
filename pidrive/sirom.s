;****************************************
;*
;* SIDRIVE (SERIAL INTERFACE DRIVE) ROM
;*
;****************************************

.DEFINE	SIG	"SI"

	.INCLUDE "romincs.s"
	
;****************************************
;*
;*      OPTION ROM SPACE @ $C800
;*
;****************************************
	ORG	$C800

	.INCLUDE "c8rom.s"
;*
;* FILL REMAINING ROM WITH 0'S
;*
	.REPEAT $CF00-*
	DB	$00
	.ENDREP
	.ASSERT	* = $CF00, error, "Code not page size"

;****************************************
;*
;*   SLOT INDEPENDENT ROM CODE @ $Cn00
;*
;****************************************
	ORG	$C700		; EASY SLOT ADDRESS TO ASSEMBLE

	.INCLUDE "cxrom.s"
