; -------------------------------------------------------------------------
;	Algorithm to calculate Easter Sunday for a given year:
;
;	century = year/100
;	G = year % 19
;	K = (century - 17)/25
;	I = (century - century/4 - (century - K)/3 + 19*G + 15) % 30
;	I = I - (I/28)*(1 - (I/28)*(29/(I + 1))*((21 - G)/11))
;	J = (year + year/4 + I + 2 - century + century/4) % 7
;	L = I - J
;	EasterMonth = 3 + (L + 40)/44
;	EasterDay = L + 28 - 31*(EasterMonth/4)
; -------------------------------------------------------------------------
CalculateEaster proc (year.UInt16)
    	beginproc
	movem.l	d3-d6,-(a7)
    
; D0 = year
	moveq.l	#0,d0
	move	year(a6),d0	; D0 = year

; D1 = D0 / 100
	move.l	d0,d1
	divu	#100,d1
	ext.l	d1		; D1 = century

; D2 = D0 modulo 19
	move.l	d0,d2
	divu	#19,d2
	swap	d2		; D2 = year modulo 19

; D3 = D2 * 19
	moveq.l	#19,d3		; D3 = 19
	mulu.w	d2,d3		; D3 = 19*D2
; D3 = D3 + 15
	add.w	#15,d3		; D3 = 19*D2+15

; D4 = D0 / 4
	move.l	d0,d4
	lsr.l	#2,d4
; D0 = D0 + D4
	add.l	d4,d0		; D0 = year + year / 4

; D4 = D1 - 17
	moveq.l	#-17,d4
	add.l	d1,d4
; D4 = D4 / -25
	divs	#-25,d4
; D4 = D4 + D1
	add	d1,d4
; D4 = D4 % 3
	ext.l	d4
	divs	#3,d4
; D3 = D3 - D4
	sub	d4,d3		; D3 = D3 - (century - (century-17)/25) / 3


; D4 = D1 % 4
	move.l	d1,d4
	lsr.l	#2,d4
; D1 = D1 - D4
	sub.l	d4,d1		; D1 = century - century/4
	

; D3 = D3 + D1
	add	d1,d3		; D3 = D3 + century - century/4

; D3 = D3 modulo 30
	ext.l	d3
	divs	#30,d3
	swap	d3		; D3 = D3 modulo 30

; D4 = D3 % 28
	move	d3,d4
	ext.l	d4
	divs	#28,d4		; D4 = D3 / 28

; D5 = 29
	moveq.l	#29,d5
; D6 = D3 + 1
	move	d3,d6
	addq.w	#1,d6
; D5 = D5 % D6
	divs	d6,d5		; D5 = 29 / (D3+1)

; D2 = D2 - 21
	subi	#21,d2
; D2 = D2 % -11
	ext.l	d2
	divs	#-11,d2		; D2 = (year modulo 19 - 21)/11

; D5 = D5 * D2
	muls	d2,d5
; D5 = D5 * D4
	muls	d4,d5
; D5 = D5 * D4
	muls	d4,d5
; D3 = D3 - D4
	sub	d4,d3
; D3 = D3 + D5
	add	d5,d3		; D3 = D3 - (D3/28) + (D3/28)*(D3/28)*(29/(D3+1))*((21 - year modulo 19)/11)

; D4 = D0 + D3 + 2 - D1
	add	d0,d4
	add	d3,d4
	addq	#2,d4
	sub	d1,d4
; D4 = D4 modulo 7
	ext.l	d4
	divs	#7,d4
	swap	d4
; D3 = D3 - D4
	sub	d4,d3		; D3 = D3 - (year + year/4 + D3 + 2 - century + century/4) modulo 7

; D1 = D3 + 40
	moveq.l	#40,d1
	add	d3,d1
	ext.l	d1
; D1 = D1 / 44
	divs	#44,d1
; D1 = D1 + 3
	addq	#3,d1		; D1.w = Easter Month

; D0 = D1 % 4
	move	d1,d0
	lsr	#2,d0
; D0 = D0 * -31
	muls	#-31,d0
; D0 = D0 + 28
	add	#28,d0
; D0 = D0 + D3
	add	d3,d0		; D0.w = Easter Day

	movem.l (a7)+,d3-d6
	endproc
