strcmp	proc	(first.char*,second.char*).UInt8
	beginproc
	movem.l	a0-a1,-(a7)
	movea.l	first(a6),a0
	movea.l	second(a6),a1
loop
	move.b	(a1)+,d0
	sub.b	(a0)+,d0
	bcs.s	returnMinus
	bhi.s	returnPlus
	tst.b	-1(a0)
	bne.s	loop
	bra.s	return
returnPlus
	moveq.l	#1,d0
	bra.s	return
returnMinus
	moveq.l	#-1,d0
return
	movem.l	(a7)+,a0-a1
	endproc