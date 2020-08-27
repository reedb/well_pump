
	include "Sony/SonyCLIE.inc"

; Err error = GetSonyLibRefNum(&refNum,sonySysFtrSysInfoLibrFm,sonySysLibNameSound,sonySysFileCSoundLib);
; Err error = GetSonyLibRefNum(&refNum,sonySysFtrSysInfoLibrHR,sonySysLibNameHR,sonySysFileCHRLib);

GetSonyLibRefNum proc (refNum.UInt16*, libFlag.UInt32, libName.char*, libCreator.UInt32).Err
	local	sonySysFtrSysInfoP.SonySysFtrSysInfoP
	beginproc
        call	FtrGet(#sonySysFtrCreator,#sonySysFtrNumSysInfoP,&sonySysFtrSysInfoP(a6))
        tst.w	d0
        bne.s	done				; just return the error code
        movea.l	sonySysFtrSysInfoP(a6),a0
        move.l	SonySysFtrSysInfoType.libr(a0),d1
        move.w	#sysErrLibNotFound,d0		; load error code in case the library is not available
        and.l	libFlag(a6),d1			; library available?
        beq.s	done				; nope
        call	SysLibFind(libName(a6),refNum(a6))
        cmp	#sysErrLibNotFound,d0		; library not loaded yet?
        bne.s	done				; either everything was okay or some other error occurred
        call	SysLibLoad(#'libr',libCreator(a6),refNum(a6));
done
	endproc
