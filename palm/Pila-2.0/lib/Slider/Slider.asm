	ifdef	SliderInitialize	; include only if really needed

; ---------------------------------------------------------------------------
;	Global variables used to keep state of slider management
;
	data

	global	sliderList.SliderType*
	global	sliderScreenSizeFactor.UInt16

	code

; ---------------------------------------------------------------------------
;	Structure of slider structure allocated for each slider
;
 struct SliderUniqueIdsType
	form.UInt16			; resource ID of form this slider belongs to
	slider.UInt16			; resource ID of slider gadget
 endstruct

 union	SliderUniqueIdType
	id.SliderUniqueIdsType
	ident.UInt32
 endunion
	
 struct SliderType
	next.SliderType*		; pointer of next slider structure (MUST be FIRST entry in struct)
	callback.void*			; pointer to call back routine called whenever slider value changes
	uniqueId.SliderUniqueIdType	; unique id of slider for later identification
	fontId.FontID			; font ID for flag
 	screenBounds.RectangleType	; coordinates on the 160x160 screen
 	realBounds.RectangleType	; coordinates adjusted for resolution
	trackBmpPtr.BitmapPtr		; pointer to bitmap for slider background
	flagBmpPtr.BitmapPtr		; pointer to bitmap for slider flag
	flagWinHndl.WinHandle		; handle to bitmap window for save-behind of slider flag
	flagHeight.UInt16		; height of slider flag
	flagPosition.UInt16		; current position of slider flag in real pixel screen coordinates
	sensitivity.UInt16		; sensitivity for non-proportional pen strokes
	text.PointType			; offset of text inside slider flag
	topMargin.UInt16		; top margin for min position
	botMargin.UInt16		; bottom margin for max position
	minValue.UInt16			; minimum value of slider
	range.UInt16			; value range of slider
	currValue.UInt16		; current value of slider
 endstruct

; ---------------------------------------------------------------------------
;	Initialize slider management
;
SliderInitialize proc (screenSizeFactor.UInt16).void
	beginproc
	clr.l	sliderList(a5)	; no slider defined yet
	move.w	screenSizeFactor(a6),sliderScreenSizeFactor(a5)
	endproc

; ---------------------------------------------------------------------------
;	Create and initialize a slider structure for the currently active form
;
SliderCreateSlider proc (sliderId.UInt16,trackBmpId.DmResID,flagBmpId.DmResID,fontId.FontID,textX.UInt16,textY.UInt16,topMargin.UInt16,botMargin.UInt16,minValue.UInt16,range.UInt16,sensitivity.UInt16).Err
	local	error.UInt16
	beginproc
	move.l	a4,-(a7)
	call	SliderAllocateStruct(sliderId(a6))	; will allocate memory and set all parm-indendent values
	move.l	a0,d0					; check for error
	beq	fatalMemError				; allocation error!
	movea.l	a0,a4					; keep pointer accessible
	call	DmGetResource(#'Tbmp',trackBmpId(a6))
	move.l	a0,d0
	bne	.1f
fatalResError
	call	DmGetLastErr()
	bra	return
fatalWinError
	move.w	error(a6),d0
	bra	return
fatalMemError
	move.w	#memErrNotEnoughSpace,d0
	bra	return
.1
	call	MemHandleLock(a0)
	move.l	a0,SliderType.trackBmpPtr(a4)
	call	DmGetResource(#'Tbmp',flagBmpId(a6))
	move.l	a0,d0
	beq	fatalResError
	call	MemHandleLock(a0)
	move.l	a0,SliderType.flagBmpPtr(a4)	; pointer to bitmap
	call	BmpGetDimensions(a0,#0,&SliderType.flagHeight(a4),#0)
	move	fontId(a6),SliderType.fontId(a4)	; font for slider flag
	move	textX(a6),SliderType.text.x(a4)		; x-coordinate of text in flag bitmap
	move	textY(a6),SliderType.text.y(a4)		; y-coordinate of text in flag bitmap
	move	topMargin(a6),SliderType.topMargin(a4)	; top margin of track
	move	botMargin(a6),SliderType.botMargin(a4)	; bottom margin of track
	move	minValue(a6),d0
	move	d0,SliderType.minValue(a4)
	move	d0,SliderType.currValue(a4)
	move	range(a6),SliderType.range(a4)
	move	sensitivity(a6),SliderType.sensitivity(a4)
	move	SliderType.flagHeight(a4),d0
	addq	#2,d0	; add 2 to avoid hi-res-problems when removing the flag
	call	WrapWinCreateOffscreenWindow(SliderType.realBounds.extent.x(a4),d0,#screenFormat,&error(a6))
	move.l	a0,SliderType.flagWinHndl(a4)	; handle of bitmap window
	beq	fatalWinError

	moveq.l	#0,d0					; no error!
return
	movea.l	(a7)+,a4
	endproc

; ---------------------------------------------------------------------------
;	Set new callback routine for slider on current form, returns old address
;
SliderSetCallback proc (sliderId.UInt16,callback.void*).void*
	beginproc
	call	SliderGetStructPointer(sliderId(a6))
	move.l	a0,d0
	beq	return	; slider structure not found
	movea.l	callback(a6),a1
	move.l	a1,d0
	bne	.1f
	lea	SliderDefaultCallback(pc),a1
.1
	move.l	SliderType.callback(a0),d0
	move.l	a1,SliderType.callback(a0)
	lea	SliderDefaultCallback(pc),a0
	cmp.l	d0,a0
	bne	.2f
	moveq.l	#0,d0
.2
	move.l	d0,a0
return
	endproc

; ---------------------------------------------------------------------------
;	Set new slider value
;
SliderSetValue proc (sliderId.UInt16,newValue.UInt16).void
	beginproc
	call	SliderGetStructPointer(sliderId(a6))
	move.l	a0,d0
	beq	return	; slider structure not found
	call	SliderMoveFlag(a0,newValue(a6))
return
	endproc

; ---------------------------------------------------------------------------
;	Get slider value
;
SliderGetValue proc (sliderId.UInt16).UInt16
	beginproc
	call	SliderGetStructPointer(sliderId(a6))
	move.l	a0,d0
	beq	return	; slider structure not found
	move.w	SliderType.currValue(a0),d0
return
	endproc

; ---------------------------------------------------------------------------
;	Draw all sliders for the currently active form
;
SliderDrawSliders proc ().void
	local	formId.UInt16
	beginproc
	move.l	a3,-(a7)
	call	FrmGetActiveFormID()
	move.w	d0,formId(a6)				; save form ID
	lea	sliderList-SliderType.next(a5),a3	; address of next slider list entry
listLoop
	movea.l	SliderType.next(a3),a3			; get pointer to next entry
	move.l	a3,d1					; and check if there is a next one
	beq	return					; done if there isn't any next entry
	move.w	formId(a6),d0				; index for list-search
	cmp.w	SliderType.uniqueId.id.form(a3),d0	; is this one we are looking for?
	bne	listLoop				; nope - go to the next entry
	call	WrapWinDrawBitmap(SliderType.trackBmpPtr(a3),SliderType.realBounds.topLeft.x(a3),SliderType.realBounds.topLeft.y(a3))
	call	SliderDrawFlag(a3)
	bra	listLoop				; and go to check the next one
return
	movea.l	(a7)+,a3
	endproc

; ---------------------------------------------------------------------------
;	Delete all sliders for the currently active form
;
SliderDeleteSliders proc ().void
	local	formId.UInt16
	beginproc
	movem.l	a2-a4,-(a7)
	call	FrmGetActiveFormID()
	move.w	d0,formId(a6)
	lea	sliderList-SliderType.next(a5),a3	; address of next slider list entry
listLoop
	movea.l	a3,a4					; keep pointer to last entry for unlinking
	movea.l	SliderType.next(a3),a3			; get pointer to next entry
	move.l	a3,d1					; and check if there is a next one
	beq	return					; done if there isn't any next entry
	move.w	formId(a6),d0				; index for list-search
	cmp.w	SliderType.uniqueId.id.form(a3),d0	; is this one we are looking for?
	bne	listLoop				; nope - go to the next entry
	move.l	SliderType.next(a3),SliderType.next(a4)	; make previous struct point to next struct
	exg	a3,a4					; and have a3 point to previous struct again
							; while a4 now points to the struct to be freed
	call	WinDeleteWindow(SliderType.flagWinHndl(a4),#0)	; delete background bitmap window
	call	MemPtrRecoverHandle(SliderType.trackBmpPtr(a4))	; recover handle from bitmap pointer
	move.l	a0,a2					; save for release after unlock
	call	MemHandleUnlock(a2)			; unlock resource memory handle
	call	DmReleaseResource(a2)			; and release resource
	call	MemPtrRecoverHandle(SliderType.flagBmpPtr(a4))
	move.l	a0,a2					; save for release after unlock
	call	MemHandleUnlock(a2)			; unlock resource memory handle
	call	DmReleaseResource(a2)			; and release resource
	call	MemPtrFree(a4)				; finally free slider structure itself
	bra	listLoop
return
	movem.l	(a7)+,a2-a4
	endproc

; ---------------------------------------------------------------------------
;	Handle pen down event and if it is inside a slider
;	track the pen until it is lifted again and dynamically
;	change the slider value if the pen is dragged
;
SliderHandlePenDownEvent proc (event.EventType*).void
	local	startY.UInt16
	local	lastY.UInt16
	local	startVal.UInt16
	local	penX.UInt16
	local	penY.UInt16
	local	repeatTicks.UInt16
	local	penDown.Boolean
	local	proportional.Boolean
	local	penMoved.Boolean
	beginproc
	movem.l	d3/a3,-(a7)
	call	FrmGetActiveFormID()
	move.w	d0,d1					; save form ID
	move.l	event(a6),a0				; get address of event structure
	lea	sliderList-SliderType.next(a5),a3	; address of next slider list entry
listLoop
	movea.l	SliderType.next(a3),a3			; get pointer to next entry
	move.l	a3,d0					; and check if there is a next one
	beq	return					; done if there isn't any next entry
	cmp.w	SliderType.uniqueId.id.form(a3),d1	; is this one we are looking for?
	bne	listLoop				; nope - go to the next entry
; check if pen went down inside this slider
	move.w	EventType.screenY(a0),d0
	sub.w	SliderType.screenBounds.topLeft.y(a3),d0
	bmi	listLoop
	cmp.w	SliderType.screenBounds.extent.y(a3),d0
	bpl	listLoop
	move.w	EventType.screenX(a0),d0
	sub.w	SliderType.screenBounds.topLeft.x(a3),d0
	bmi	listLoop
	cmp.w	SliderType.screenBounds.extent.x(a3),d0
	bpl	listLoop
; pen did go down inside this slider
	clr.b	proportional(a6)
	move.w	EventType.screenY(a0),d0
	move.w	d0,startY(a6)				; save the start position of the pen stroke
	move.w	d0,lastY(a6)				; and also make it the last seen position
	mulu	sliderScreenSizeFactor(a5),d0		; make into real pixels
	sub.w	SliderType.flagPosition(a3),d0
	bcs.s	notProportional				; jump if tapped above the flag
	cmp.w	SliderType.flagHeight(a3),d0
	bcc.s	notProportional				; jump if tapped below the flag
	move.b	#-1,proportional(a6)
notProportional
	clr.b	penMoved(a6)				; used to recognize tapped-only and auto-repeat strokes
	move.w	SliderType.currValue(a3),startVal(a6)	; save the current value of the slider as start value
; setup repeat-delay and repeat timings
	call	TimGetTicks()				; current tick count
	move.l	d0,d3					; save for calculation
	call	SysTicksPerSecond()
	ext.l	d0
	lsr.l	#1,d0					; ticks per half second
	add.l	d0,d3					; half a second delay until tap-repeat
	lsr.l	#3,d0					; and 16 repeats per second afterwards
	bne.s	.1f					; in case there are less than 16 ticks per second
	moveq.l	#1,d0					; make it repeat with every tick
.1	move.w	d0,repeatTicks(a6)
penLoop
	call	EvtGetPen(&penX(a6),&penY(a6),&penDown(a6))
	move.w	lastY(a6),d0				; avoid high-speed re-draw loop by suppressing
	move.w	penY(a6),lastY(a6)			; re-calculation and re-drawing if the pen
	cmp.w	penY(a6),d0				; did not move since the last time around
	beq.s	checkAutoRepeat
	call	SliderCalcValue(a3,startVal(a6),startY(a6),penY(a6),proportional(a6))
	cmp.w	SliderType.currValue(a3),d0
	bne.s	penHasMoved
checkAutoRepeat
	tst.b	penMoved(a6)
	bne.s	checkPen				; jump if pen has already been dragged (no tap-repeat anymore)
	call	TimGetTicks()
	cmp.l	d0,d3
	bcs.s	autoTap					; if wait time is up do as if the pen was tapped
	tst.b	penDown(a6)
	bne.s	penLoop					; loop until pen is lifted - otherwise do one tap!
autoTap
	move.w	repeatTicks(a6),d0
	ext.l	d0
	add.l	d0,d3
	call	SliderTapped(a3,startY(a6))
	move.w	SliderType.currValue(a3),startVal(a6)	; use current value as new start value if pen is dragged now
	bra.s	checkPen
penHasMoved
	move.b	#1,penMoved(a6)				; remember that the pen was dragged
	call	SliderMoveFlag(a3,d0)			; move slider flag to new value
checkPen
	tst.b	penDown(a6)
	bne	penLoop					; loop until pen is lifted
; pen is up so we are done tracking it
	call	EvtFlushPenQueue()			; empty the pen queue
	moveq.l	#1,d0					; signal to caller that the event was handled
return
	movem.l	(a7)+,d3/a3
	endproc

; ----------------------------------------------------------------------------
;	Add/subtract one to/from current slider value depending on the
;	tap being below (add) or above (subtract) the flag. If the pen is
;	inside the flag, nothing is changed.
;
SliderTapped proc (slider.SliderType*,tapPos.UInt16).void
	beginproc
	movea.l	slider(a6),a0				; get access to slider structure
	move.w	SliderType.currValue(a0),d1
	move.w	tapPos(a6),d0
	mulu	sliderScreenSizeFactor(a5),d0		; make into real pixels
	sub.w	SliderType.flagPosition(a0),d0
	bcs.s	decrementValue				; jump if tapped above the flag
	cmp.w	SliderType.flagHeight(a0),d0
	bcs.s	noChange				; jump if tapped inside the flag
incrementValue
	addq.w	#2,d1
decrementValue
	subq.w	#1,d1
	call	SliderMoveFlag(a0,d1)
noChange
	endproc

; ---------------------------------------------------------------------------
;	Move slider flag to passed-in position (erase old flag first and draw new flag)
;	1. the new value is ensured to be within the allowed range.
;	2. the callback routine is called.
;	3. if the value returned by the callback routine is the same as the current
;	   value of the slider, nothing is done and the function just returns.
;	4. otherwise the old slider flag is erased
;	5. the new slider flag is drawn
;
SliderMoveFlag proc (slider.SliderType*,value.UInt16).void
	local	srcRect.RectangleType
	beginproc
	movea.l	slider(a6),a0
	move.w	value(a6),d0				; get new slider value and ensure it is inside the allowed values
	move.w	SliderType.minValue(a0),d1
	cmp.w	d1,d0
	bge.s	.1f					; jump if not lower than minimum
	move.w	d1,d0					; otherwise set to minimum
.1	add.w	SliderType.range(a0),d1			; calculate maximum+1
	cmp.w	d1,d0
	blt.s	.1f					; jump if lower than maximum+1
	move.w	d1,d0
	subq.w	#1,d0					; otherwise set to maximum
.1	; value in d0 is now within the allowed range!
; run callback with new slider value as parameter
	move.w	SliderType.range(a0),-(a7)		; build parameter list for callback on the stack:
	move.w	SliderType.minValue(a0),-(a7)		; callback(value,minValue,range)
	move.w	d0,-(a7)
	movea.l	SliderType.callback(a0),a0		; get address of callback routine
	jsr	(a0)					; call callback routine
	addq.l	#6,a7					; pop  parameters off the stack
; d0 now contains the new value for the slider (possibly modified by the callback routine)
	movea.l	slider(a6),a0
	cmp.w	SliderType.currValue(a0),d0		; check if we really have a new value
	beq.s	return					; if it is still the same as before just return
	move.w	d0,SliderType.currValue(a0)		; otherwise store the new value and continue
; erase old slider flag
	clr.l	srcRect.topLeft(a6)
	move.w	SliderType.realBounds.extent.x(a0),srcRect.extent.x(a6)
	move.w	SliderType.flagHeight(a0),srcRect.extent.y(a6)
	addq.w	#2,srcRect.extent.y(a6) ; add 2 to avoid hi-res problem when removing flag
	move.w	SliderType.flagPosition(a0),d0
	subq.w	#1,d0			; added height split between above and underneath
	call	WrapWinCopyRectangle(SliderType.flagWinHndl(a0),#0,&srcRect(a6),SliderType.realBounds.topLeft.x(a0),d0,#winPaint)
; draw new slider
	call	SliderDrawFlag(slider(a6))
return
	endproc

; --------------------------------------------------------------------------
;	Draw the slider flag at the position of the current value.
;	For that SliderType.flagPosition is re-calculated and stored!
;
SliderDrawFlag proc (slider.SliderType*).void
	local	srcRect.RectangleType
	local	valueString.char[2]
	beginproc
	move.l	a3,-(a7)
	movea.l	slider(a6),a3				; keep slider struct accessible
; calculate flag position for current value
	move.w	SliderType.currValue(a3),d0
	sub.w	SliderType.minValue(a3),d0
	move.w	SliderType.realBounds.extent.y(a3),d1	; calculate possible number of flag positions
	sub.w	SliderType.topMargin(a3),d1
	sub.w	SliderType.botMargin(a3),d1
	sub.w	SliderType.flagHeight(a3),d1		; number of possible flag positions-1
	ble.s	.1f					; zero or negative? then skip multiplication
	mulu.w	d1,d0					; multiply value proportion with number of possible slider flag positions-1
.1	ext.l	d0
	move.w	SliderType.range(a3),d1
	subq.w	#1,d1
	divu	d1,d0					; divide by number of different values-1
	add.w	SliderType.topMargin(a3),d0		; add top margin -> position relative to slider origin
	add.w	SliderType.realBounds.topLeft.y(a3),d0	; y-position of top of flag in real pixels on screen
	move.w	d0,SliderType.flagPosition(a3)		; store new flag position in structure
; save screen content underneath the flag first
	move.w	SliderType.realBounds.topLeft.x(a3),srcRect.topLeft.x(a6)
	subq.w	#1,d0			; save line above the flag as well
	move.w	d0,srcRect.topLeft.y(a6)
	move.w	SliderType.realBounds.extent.x(a3),srcRect.extent.x(a6)
	move.w	SliderType.flagHeight(a3),srcRect.extent.y(a6)
	addq.w	#2,srcRect.extent.y(a6)	; add 2 to save one line above and one underneath
	call	WrapWinCopyRectangle(#0,SliderType.flagWinHndl(a3),&srcRect(a6),#0,#0,#winPaint)
; and now draw the flag and write the text into it
	call	WrapWinDrawBitmap(SliderType.flagBmpPtr(a3),srcRect.topLeft.x(a6),srcRect.topLeft.y(a6))
	move.w	SliderType.currValue(a3),d0
	ext.l	d0
	divs	#10,d0
	addi.w	#'0',d0
	move.b	d0,valueString+0(a6) ; +0 to suppress warning about size mismatch
	swap	d0
	addi.w	#'0',d0
	move.b	d0,valueString+1(a6)
	call	WrapFntSetFont(SliderType.fontId(a3))	; set custom font
	move.w	d0,-(a7)				; save former font ID for later reset
	move.w	SliderType.realBounds.topLeft.x(a3),d0	; x-position of flag
	add.w	SliderType.text.x(a3),d0		; x-position of text
	move.w	srcRect.topLeft.y(a6),d1		; y-position of flag
	add.w	SliderType.text.y(a3),d1		; y-position of text
	call	WrapWinDrawChars(&valueString(a6),#2,d0,d1)
	move.w	(a7)+,d0				; retrieve original font ID
	call	WrapFntSetFont(d0)			; reset font
	movea.l	(a7)+,a3
	endproc

; ---------------------------------------------------------------------------
;	Calculate slider value from a base value and a start and
;	an end position of a pen stroke in screen coordinates
;	If the flag proportional is set or SliderType.sensitivity is zero,
;	then the slider flag position is synchronized with the pen.
;	Otherwise moving the pen down/up for SliderType.sensitivity screen
;	points will add/subtract one to/from the current slider value.
;
SliderCalcValue proc (sliderStruct.SliderType*,baseValue.UInt16,startY.UInt16,endY.UInt16,proportional.Boolean).UInt16
	beginproc
	movea.l	sliderStruct(a6),a0
	move.w	endY(a6),d0
	sub.w	startY(a6),d0
	tst.b	proportional(a6)
	bne	calcProportional
; calculate delta based on sensitivity value
	move.w	SliderType.sensitivity(a0),d1
	beq.s	calcProportional
	ext.l	d0
	divs	d1,d0
	bra.s	deltaCalculated
; calculate delta proportional to slider track
calcProportional
	muls	sliderScreenSizeFactor(a5),d0	; make delta into real pixels
	move.w	SliderType.range(a0),d1
	subq.w	#1,d1
	muls	d1,d0
	move.w	SliderType.realBounds.extent.y(a0),d1
	sub.w	SliderType.topMargin(a0),d1
	sub.w	SliderType.botMargin(a0),d1
	sub.w	SliderType.flagHeight(a0),d1
	divs	d1,d0
deltaCalculated
	add.w	baseValue(a6),d0		; calculate target value and make sure it is inside the allowed range
	move.w	SliderType.minValue(a0),d1
	cmp.w	d1,d0
	bge.s	.1f				; jump if greater or equal minimum value
	move.w	d1,d0				; otherwise set to minimum value
.1	add.w	SliderType.range(a0),d1		; d1 is now maximum value+1
	cmp.w	d1,d0
	blt	return				; jump if lower than maximum value+1
	move.w	d1,d0
	subq.w	#1,d0				; set to maximum value
return
	endproc

; ---------------------------------------------------------------------------
;	Find structure for a given slider
;
SliderGetStructPointer proc (sliderId.UInt16)
	beginproc
	call	FrmGetActiveFormID()
	swap	d0
	move.w	sliderId(a6),d0				; index for list-search
	lea.l	sliderList-SliderType.next(a5),a0	; address pointing to address of next slider structure
listLoop
	movea.l	SliderType.next(a0),a0			; get pointer to next entry
	move.l	a0,d1
	beq	return					; done if there isn't any next entry
	cmp.l	SliderType.uniqueId.ident(a0),d0	; is this the one we are looking for?
	bne	listLoop				; nope - go to the next entry
return
	endproc

; ---------------------------------------------------------------------------
;	Allocate new slider structure
;
SliderAllocateStruct proc (sliderId.UInt16).SliderType*
	beginproc
	call	SliderGetStructPointer(sliderId(a6))	; check if the slider already exists
	move.l	a0,d0
	bne.s	.1f					; and use that structure if so
	call	MemPtrNew(#sizeof(SliderType))		; allocate memory for slider structure
	move.l	a0,d0					; check the pointer
	beq	return					; allocation error
.1
	move.l	sliderList(a5),SliderType.next(a0)	; save ptr to current first entry
	move.l	a0,sliderList(a5)			; store new ptr to current first entry
	move.w	sliderId(a6),SliderType.uniqueId.id.slider(a0)	; save slider resource ID
	call	FrmGetActiveFormID()
	move.l	sliderList(a5),a0
	move.w	d0,SliderType.uniqueId.id.form(a0)
	lea	SliderDefaultCallback(pc),a1
	move.l	a1,SliderType.callback(a0)		; set default call back routine
	call	FrmGetActiveForm()
	move.l	a0,-(a7)				; save pointer to form for next call
	call	FrmGetObjectIndex(a0,sliderId(a6))
	move.l	(a7)+,d1				; get pointer to form back
	move.l	sliderList(a5),a0			; get pointer to structure
	call	FrmGetObjectBounds(d1,d0,&SliderType.screenBounds(a0))
	move.l	sliderList(a5),a0
	move.w	sliderScreenSizeFactor(a5),d1
	move.w	SliderType.screenBounds.topLeft.x(a0),d0
	mulu	d1,d0
	move.w	d0,SliderType.realBounds.topLeft.x(a0)
	move.w	SliderType.screenBounds.topLeft.y(a0),d0
	mulu	d1,d0
	move.w	d0,SliderType.realBounds.topLeft.y(a0)
	move.w	SliderType.screenBounds.extent.x(a0),d0
	mulu	d1,d0
	move.w	d0,SliderType.realBounds.extent.x(a0)
	move.w	SliderType.screenBounds.extent.y(a0),d0
	mulu	d1,d0
	move.w	d0,SliderType.realBounds.extent.y(a0)
return
	endproc

; ---------------------------------------------------------------------------
; Default callback routine just returning the input value
;
SliderDefaultCallback proc (newValue.UInt16,minValue.UInt16,range.UInt16).UInt16
	beginproc
	move.w	newValue(a6),d0
	endproc

	endif
