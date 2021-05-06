; Startup.inc
; by Darrin Massena (darrin@massena.com)
; 17 Jul 96
; modified for Pila V2.0 by Frank Schaeckermann (frmipg602@sneakemail.com)
; 23 June 2003

        code

__Startup__ proc ()
	local pappi.l
	local prevGlobals.l
	local globalsPtr.l
	beginproc
        call	SysAppStartup(&pappi(a6), &prevGlobals(a6), &globalsPtr(a6))
        tst.w   d0
        beq.s   .1f
        call	SndPlaySystemSound(#sndError)
        moveq   #-1,d0
        bra.s   .2f
.1	movea.l pappi(a6),a0
        call	PilotMain(SysAppInfoType.cmd(a0), SysAppInfoType.cmdPBP(a0), SysAppInfoType.launchFlags(a0))
        call	SysAppExit(pappi(a6), prevGlobals(a6), globalsPtr(a6))
        moveq   #0,d0
.2	endproc

        data
        ds.l    8 ; Palm OS uses the first 32 bytes of (a5) for itself
        code
