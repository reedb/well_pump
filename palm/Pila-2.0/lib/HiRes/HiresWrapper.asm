	include	"Sony/GetSonyLibRefNum.asm"

	data
	global	sonyHRLibRefNum.UInt16
	global	hiresMode.Boolean
	align	2
	code
	
WrapHiresInit proc ().Boolean
	local	width.UInt32
	local	height.UInt32
	beginproc
; Check if we are on Palm OS 5 and have to stick with lowres for now
	call	SysGetOSVersionString()
	move.b	3(a0),-(a7)
	call	MemPtrFree(a0)
	move.b	(a7)+,d0
	cmp.b	#'5',d0
	bcs	tryHighresMode
; we are on Palm OS 5 and better be using the Palm OS High Density Support
	bra	setLowresMode ; and since we can't yet use lowres
tryHighresMode
; load Sony HR Library for hires support
        call	GetSonyLibRefNum(&sonyHRLibRefNum(a5),#sonySysFtrSysInfoLibrHR,&sonySysLibNameHR(a5),#sonySysFileCHRLib)
	tst.w	d0
	bne.s	setLowresMode
setHiresMode
	move.l	#320,d0
	move.l	d0,width(a6)
	move.l	d0,height(a6)
	call	HROpen(sonyHRLibRefNum(a5))
        call	HRWinScreenMode(sonyHRLibRefNum(a5),#winScreenModeSet,&width(a6),&height(a6),#0,#0)
	tst.b	d0
	beq.s	hiresModeOkay
	move.l	#480,height(a6)
	call	HRWinScreenMode(sonyHRLibRefNum(a5),#winScreenModeSet,&width(a6),&height(a6),#0,#0)
	tst.b	d0
	bne.s	setLowresMode
hiresModeOkay
	moveq.l	#1,d0
	bra.s	storeMode
setLowresMode
	moveq.l	#0,d0
storeMode
	move.b	d0,hiresMode(a5)
	endproc
	
WrapHiresFinish proc ().Err
	beginproc
	tst.b	hiresMode(a5)
	beq.s	return
	call	HRWinScreenMode(sonyHRLibRefNum(a5),#winScreenModeSetToDefaults,#0,#0,#0,#0)
	call	HRClose(sonyHRLibRefNum(a5))
return	endproc

	ifdef	WrapFntGetFont
WrapFntGetFont proxy ().FontID
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapFntGetFont
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapFntGetFont
return	endproxy
	endif

	ifdef	WrapFntSetFont
WrapFntSetFont proxy (font.FontID).FontID
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapFntSetFont
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapFntSetFont
return	endproxy
	endif

	ifdef	WrapWinEraseRectangle	
WrapWinEraseRectangle proxy (rP.RectangleType*,cornerDiam.UInt16).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinEraseRectangle
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinEraseRectangle
return	endproxy
	endif
	
	ifdef	WrapWinDrawChar	
WrapWinDrawChar proxy (theChar.WChar,x.Coord,y.Coord).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinDrawChar
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinDrawChar
return	endproxy
	endif
	
	ifdef	WrapWinDrawChars	
WrapWinDrawChars proxy (chars.Char*,len.Int16,x.Coord,y.Coord).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinDrawChars
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinDrawChars
return	endproxy
	endif
	
	ifdef	WrapWinDrawLine	
WrapWinDrawLine proxy (x1.Coord,y1.Coord,x2.Coord,y2.Coord).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinDrawLine
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinDrawLine
return	endproxy
	endif

	ifdef	WrapWinDrawBitmap
WrapWinDrawBitmap proxy (bitmapP.BitmapPtr,x.Coord,y.Coord).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinDrawBitmap
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinDrawBitmap
return	endproxy
	endif

	ifdef	WrapWinDrawRectangleFrame
WrapWinDrawRectangleFrame proxy (frameType.FrameType,rect.RectangleType*).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinDrawRectangleFrame
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinDrawRectangleFrame
return	endproxy
	endif

	ifdef	WrapWinDrawRectangle
WrapWinDrawRectangle proxy (rect.RectangleType*,cornerDiam.UInt16).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinDrawRectangle
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinDrawRectangle
return	endproxy
	endif

	ifdef	WrapWinCreateBitmapWindow	
WrapWinCreateBitmapWindow proxy (bitmapP.BitmapType*,error.UInt16*).WinHandle
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinCreateBitmapWindow
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinCreateBitmapWindow
return	endproxy
	endif

	ifdef	WrapWinCreateOffscreenWindow	
WrapWinCreateOffscreenWindow proxy (width.Coord,height.Coord,format.WindowFormatType,error.UInt16*).WinHandle
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinCreateOffscreenWindow
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinCreateOffscreenWindow
return	endproxy
	endif

	ifdef	WrapWinCopyRectangle
WrapWinCopyRectangle proxy (srcWin.WinHandle,dstWin.WinHandle,srcRect.RectangleType*,destX.Coord,destY.Coord,mode.WinDrawOperation).void
	tst.b	hiresMode(a5)
	beq.s	lowres
	move.w	sonyHRLibRefNum(a5),-(a7)
	trap	#sysDispatchTrapNum
	dc.w	HRTrapWinCopyRectangle
	addq.l	#2,a7
	bra.s	return
lowres	trap	#sysDispatchTrapNum
	dc.w	sysTrapWinCopyRectangle
return	endproxy
	endif
