/***********************************************************************
 *
 *      INSTTABLE.C
 *      Instruction Table for 68000 Assembler
 *
 * Description: This file contains two kinds of data structure declara-
 *      tions: "flavor lists" and the instruction table. First
 *      in the file are "flavor lists," one for each different
 *      instruction. Then comes the instruction table, which
 *      contains the mnemonics of the various instructions, a
 *      pointer to the flavor list for each instruction, and
 *      other data. Finally, the variable tableSize is
 *      initialized to contain the number of instructions in
 *      the table.
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 12/13/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Changes for Pila Version 2.0
 *                 - added all directives (like include etc.) to the
 *                   instruction lookup-table
 *
 ************************************************************************/

/*********************************************************************

          HOW THE INSTRUCTION TABLE AND FLAVOR LISTS ARE USED

     The procedure which instLookup() and assemble() use to look up
and verify an instruction (or directive) is as follows. Once the
mnemonic of the instruction has been parsed and stripped of its size
code and trailing spaces, the instLookup() does a binary search on the
instruction table to determine if the mnemonic is present. If it is
not found, then the INV_OPCODE error results. If the mnemonic is
found, then assemble() examines the field parseFlag for that entry.
This flag is true if the mnemonic represents a normal instruction that
can be parsed by assemble(); it is false if the instruction's operands
have an unusual format (as is the case for MOVEM and DC).

      If the parseFlag is true, then assemble will parse the
instruction's operands, check them for validity, and then pass the
data to the proper routine which will build the instruction. To do
this it uses the pointer in the instruction table to the instruction's
"flavor list" and scans through the list until it finds an particular
"flavor" of the instruction which matches the addressing mode(s)
specified.  If it finds such a flavor, it checks the instruction's
size code and passes the instruction mask for the appropriate size
(there are three masks for each flavor) to the building routine
through a pointer in the flavor list for that flavor.

*********************************************************************/

#include "pila.h"
#include "asm.h"
#include "insttabl.h"
#include "directiv.h"

/* Definitions of addressing mode masks for various classes of references */

#define Data    (DnDirect | AnInd | AnIndPost | AnIndPre | AnIndDisp \
         | AnIndIndex | AbsShort | AbsLong | PCDisp | PCIndex \
         | Immediate)

#define Memory  (AnInd | AnIndPost | AnIndPre | AnIndDisp | AnIndIndex \
         | AbsShort | AbsLong | PCDisp | PCIndex | Immediate)

#define Control (AnInd | AnIndDisp | AnIndIndex | AbsShort | AbsLong | PCDisp \
         | PCIndex)

#define Alter   (DnDirect | AnDirect | AnInd | AnIndPost | AnIndPre \
         | AnIndDisp | AnIndIndex | AbsShort | AbsLong)

#define All (Data | Memory | Control | Alter)
#define DataAlt (Data & Alter)
#define MemAlt  (Memory & Alter)
#define Absolute (AbsLong | AbsShort)
#define GenReg  (DnDirect | AnDirect)


/* Define size code masks for instructions that allow more than one size */

#define BW  (BYTE | WORD)
#define WL  (WORD | LONG)
#define BWL (BYTE | WORD | LONG)
#define BL  (BYTE | LONG)


/* Define the "flavor lists" for each different instruction */

// #pragma warning disable: 4305

flavor abcdfl[] = {
    { DnDirect, DnDirect, BYTE, twoReg, 0xC100, 0xC100, 0},
    { AnIndPre, AnIndPre, BYTE, twoReg, 0xC108, 0xC108, 0}
};

flavor addfl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0600, 0x0640, 0x0680},
    { Immediate, AnDirect, WL, quickMath, 0, 0x5040, 0x5080},
    { All, DnDirect, BWL, arithReg, 0xD000, 0xD040, 0xD080},
    { DnDirect, MemAlt, BWL, arithAddr, 0xD100, 0xD140, 0xD180},
    { All, AnDirect, WL, arithReg, 0, 0xD0C0, 0xD1C0},
};

flavor addafl[] = {
    { All, AnDirect, WL, arithReg, 0, 0xD0C0, 0xD1C0},
};

flavor addifl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0600, 0x0640, 0x0680}
};

flavor addqfl[] = {
    { Immediate, DataAlt, BWL, quickMath, 0x5000, 0x5040, 0x5080},
    { Immediate, AnDirect, WL, quickMath, 0, 0x5040, 0x5080}
};

flavor addxfl[] = {
    { DnDirect, DnDirect, BWL, twoReg, 0xD100, 0xD140, 0xD180},
    { AnIndPre, AnIndPre, BWL, twoReg, 0xD108, 0xD148, 0xD188}
};

flavor andfl[] = {
    { Data, DnDirect, BWL, arithReg, 0xC000, 0xC040, 0xC080},
    { DnDirect, MemAlt, BWL, arithAddr, 0xC100, 0xC140, 0xC180},
    { Immediate, DataAlt, BWL, immedInst, 0x0200, 0x0240, 0x0280}
};

flavor andifl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0200, 0x0240, 0x0280},
    { Immediate, CCRDirect, BYTE, immedToCCR, 0x023C, 0x023C, 0},
    { Immediate, SRDirect, WORD, immedWord, 0, 0x027C, 0}
};

flavor aslfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE1C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE120, 0xE160, 0xE1A0},
    { Immediate, DnDirect, BWL, shiftReg, 0xE100, 0xE140, 0xE180}
};

flavor asrfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE0C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE020, 0xE060, 0xE0A0},
    { Immediate, DnDirect, BWL, shiftReg, 0xE000, 0xE040, 0xE080}
};

flavor bccfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6400, 0x6400, 0x6400}
};

flavor bchgfl[] = {
    { DnDirect, MemAlt, BYTE, arithAddr, 0x0140, 0x0140, 0},
    { DnDirect, DnDirect, LONG, arithAddr, 0, 0x0140, 0x0140},
    { Immediate, MemAlt, BYTE, staticBit, 0x0840, 0x0840, 0},
    { Immediate, DnDirect, LONG, staticBit, 0, 0x0840, 0x0840}
};

flavor bclrfl[] = {
    { DnDirect, MemAlt, BYTE, arithAddr, 0x0180, 0x0180, 0},
    { DnDirect, DnDirect, LONG, arithAddr, 0, 0x0180, 0x0180},
    { Immediate, MemAlt, BYTE, staticBit, 0x0880, 0x0880, 0},
    { Immediate, DnDirect, LONG, staticBit, 0, 0x0880, 0x0880}
};

flavor bcsfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6500, 0x6500, 0x6500}
};

flavor beqfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6700, 0x6700, 0x6700}
};

flavor bgefl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6C00, 0x6C00, 0x6C00}
};

flavor bgtfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6E00, 0x6E00, 0x6E00}
};

flavor bhifl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6200, 0x6200, 0x6200}
};

flavor bhsfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6400, 0x6400, 0x6400}
};

flavor blefl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6f00, 0x6F00, 0x6F00}
};

flavor blofl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6500, 0x6500, 0x6500}
};

flavor blsfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6300, 0x6300, 0x6300}
};

flavor bltfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6d00, 0x6D00, 0x6D00}
};

flavor bmifl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6b00, 0x6B00, 0x6B00}
};

flavor bnefl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6600, 0x6600, 0x6600}
};

flavor bplfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6a00, 0x6A00, 0x6A00}
};

flavor brafl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6000, 0x6000, 0x6000}
};

flavor bsetfl[] = {
    { DnDirect, MemAlt, BYTE, arithAddr, 0x01C0, 0x01C0, 0},
    { DnDirect, DnDirect, LONG, arithAddr, 0, 0x01C0, 0x01C0},
    { Immediate, MemAlt, BYTE, staticBit, 0x08C0, 0x08C0, 0},
    { Immediate, DnDirect, LONG, staticBit, 0, 0x08C0, 0x08C0}
};

flavor bsrfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6100, 0x6100, 0x6100}
};

flavor btstfl[] = {
    { DnDirect, Memory, BYTE, arithAddr, 0x0100, 0x0100, 0},
    { DnDirect, DnDirect, LONG, arithAddr, 0, 0x0100, 0x0100},
    { Immediate, Memory, BYTE, staticBit, 0x0800, 0x0800, 0},
    {    Immediate,    DnDirect,    LONG,    staticBit,    0,    0x0800,    0x0800}
};

flavor bvcfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6800, 0x6800, 0x6800}
};

flavor bvsfl[] = {
    { Absolute, 0, SHORT | LONG, branch, 0x6900, 0x6900, 0x6900}
};

flavor chkfl[] = {
    { Data, DnDirect, WORD, arithReg, 0, 0x4180, 0}
};

flavor clrfl[] = {
    { DataAlt, 0, BWL, oneOp, 0x4200, 0x4240, 0x4280}
};

flavor cmpfl[] = {
    { All, DnDirect, BWL, arithReg, 0xB000, 0xB040, 0xB080},
    { All, AnDirect, WL, arithReg, 0, 0xB0C0, 0xB1C0},
    { Immediate, DataAlt, BWL, immedInst, 0x0C00, 0x0C40, 0x0C80}
};

flavor cmpafl[] = {
    { All, AnDirect, WL, arithReg, 0, 0xB0C0, 0xB1C0}
};

flavor cmpifl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0C00, 0x0C40, 0x0C80}
};

flavor cmpmfl[] = {
    { AnIndPost, AnIndPost, BWL, twoReg, 0xB108, 0xB148, 0xB188}
};

flavor dbccfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x54C8, 0}
};

flavor dbcsfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x55C8, 0}
};

flavor dbeqfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x57C8, 0}
};

flavor dbffl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x51C8, 0}
};

flavor dbgefl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x5CC8, 0}
};

flavor dbgtfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x5EC8, 0}
};

flavor dbhifl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x52C8, 0}
};

flavor dbhsfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x54C8, 0}
};

flavor dblefl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x5FC8, 0}
};

flavor dblofl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x55C8, 0}
};

flavor dblsfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x53C8, 0}
};

flavor dbltfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x5DC8, 0}
};

flavor dbmifl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x5BC8, 0}
};

flavor dbnefl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x56C8, 0}
};

flavor dbplfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x5AC8, 0}
};

flavor dbrafl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x51C8, 0}
};

flavor dbtfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x50C8, 0}
};

flavor dbvcfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x58C8, 0}
};

flavor dbvsfl[] = {
    { DnDirect, Absolute, WORD, dbcc, 0, 0x59C8, 0}
};

flavor divsfl[] = {
    { Data, DnDirect, WORD, arithReg, 0, 0x81C0, 0}
};

flavor divufl[] = {
    { Data, DnDirect, WORD, arithReg, 0, 0x80C0, 0}
};

flavor eorfl[] = {
    { DnDirect, DataAlt, BWL, arithAddr, 0xB100, 0xB140, 0xB180},
    { Immediate, DataAlt, BWL, immedInst, 0x0A00, 0x0A40, 0x0A80}
};

flavor eorifl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0A00, 0x0A40, 0x0A80},
    { Immediate, CCRDirect, BYTE, immedToCCR, 0x0A3C, 0x0A3C, 0},
    { Immediate, SRDirect, WORD, immedWord, 0, 0x0A7C, 0}
};

flavor exgfl[] = {
    { DnDirect, DnDirect, LONG, exg, 0, 0xC140, 0xC140},
    { AnDirect, AnDirect, LONG, exg, 0, 0xC148, 0xC148},
    { GenReg, GenReg, LONG, exg, 0, 0xC188, 0xC188}
};

flavor extfl[] = {
    { DnDirect, 0, WL, oneReg, 0, 0x4880, 0x48C0}
};

flavor illegalfl[] = {
    { 0, 0, 0, zeroOp, 0, 0x4AFC, 0}
};

flavor jmpfl[] = {
    { Control, 0, 0, oneOp, 0, 0x4EC0, 0}
};

flavor jsrfl[] = {
    { Control, 0, 0, oneOp, 0, 0x4E80, 0}
};

flavor leafl[] = {
    { Control, AnDirect, LONG, arithReg, 0, 0x41C0, 0x41C0}
};

flavor linkfl[] = {
    { AnDirect, Immediate, 0, linkOp, 0, 0x4E50, 0}
};

flavor lslfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE3C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE128, 0xE168, 0xE1A8},
    { Immediate, DnDirect, BWL, shiftReg, 0xE108, 0xE148, 0xE188}
};

flavor lsrfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE2C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE028, 0xE068, 0xE0A8},
    { Immediate, DnDirect, BWL, shiftReg, 0xE008, 0xE048, 0xE088}
};

flavor movefl[] = {
    { All, DataAlt, BWL, move, 0x1000, 0x3000, 0x2000},
    { All, AnDirect, WL, move, 0, 0x3000, 0x2000},
    { Data, CCRDirect, WORD, oneOp, 0, 0x44C0, 0},
    { Data, SRDirect, WORD, oneOp, 0, 0x46C0, 0},
    { CCRDirect, DataAlt, WORD, moveReg, 0, 0x42C0, 0},
    { SRDirect, DataAlt, WORD, moveReg, 0, 0x40C0, 0},
    { AnDirect, USPDirect, LONG, moveUSP, 0, 0x4E60, 0x4E60},
    { USPDirect, AnDirect, LONG, moveUSP, 0, 0x4E68, 0x4E68}
};

flavor moveafl[] = {
    { All, AnDirect, WL, move, 0, 0x3000, 0x2000}
};

flavor movecfl[] = {
    { SFCDirect | DFCDirect | USPDirect | VBRDirect,
        GenReg, LONG, movec, 0, 0x4E7A, 0x4E7A},
    { GenReg, SFCDirect | DFCDirect | USPDirect | VBRDirect,
        LONG, movec, 0, 0x4E7B, 0x4E7B}
};

flavor movepfl[] = {
    { DnDirect, AnIndDisp, WL, movep, 0, 0x0188, 0x01C8},
    { AnIndDisp, DnDirect, WL, movep, 0, 0x0108, 0x0148},
    { DnDirect, AnInd, WL, movep, 0, 0x0188, 0x01C8},
    { AnInd, DnDirect, WL, movep, 0, 0x0108, 0x0148}
};

flavor moveqfl[] = {
    { Immediate, DnDirect, LONG, moveq, 0, 0x7000, 0x7000}
};

flavor movesfl[] = {
    { GenReg, MemAlt, BWL, moves, 0x0E00, 0x0E40, 0x0E80},
    { MemAlt, GenReg, BWL, moves, 0x0E00, 0x0E40, 0x0E80}
};

flavor mulsfl[] = {
    { Data, DnDirect, WORD, arithReg, 0, 0xC1C0, 0}
};

flavor mulufl[] = {
    { Data, DnDirect, WORD, arithReg, 0, 0xC0C0, 0}
};

flavor nbcdfl[] = {
    { DataAlt, 0, BYTE, oneOp, 0x4800, 0x4800, 0}
};

flavor negfl[] = {
    { DataAlt, 0, BWL, oneOp, 0x4400, 0x4440, 0x4480}
};

flavor negxfl[] = {
    { DataAlt, 0, BWL, oneOp, 0x4000, 0x4040, 0x4080}
};

flavor nopfl[] = {
    { 0, 0, 0, zeroOp, 0, 0x4E71, 0}
};

flavor notfl[] = {
    { DataAlt, 0, BWL, oneOp, 0x4600, 0x4640, 0x4680}
};

flavor orfl[] = {
    { Data, DnDirect, BWL, arithReg, 0x8000, 0x8040, 0x8080},
    { DnDirect, MemAlt, BWL, arithAddr, 0x8100, 0x8140, 0x8180},
    { Immediate, DataAlt, BWL, immedInst, 0x0000, 0x0040, 0x0080}
};

flavor orifl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0000, 0x0040, 0x0080},
    { Immediate, CCRDirect, BYTE, immedToCCR, 0x003C, 0x003C, 0},
    { Immediate, SRDirect, WORD, immedWord, 0, 0x007C, 0}
};

flavor peafl[] = {
    { Control, 0, LONG, oneOp, 0, 0x4840, 0x4840}
};

flavor resetfl[] = {
    { 0, 0, 0, zeroOp, 0, 0x4E70, 0}
};

flavor rolfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE7C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE138, 0xE178, 0xE1B8},
    { Immediate, DnDirect, BWL, shiftReg, 0xE118, 0xE158, 0xE198}
};

flavor rorfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE6C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE038, 0xE078, 0xE0B8},
    { Immediate, DnDirect, BWL, shiftReg, 0xE018, 0xE058, 0xE098}
};

flavor roxlfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE5C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE130, 0xE170, 0xE1B0},
    { Immediate, DnDirect, BWL, shiftReg, 0xE110, 0xE150, 0xE190}
};

flavor roxrfl[] = {
    { MemAlt, 0, WORD, oneOp, 0, 0xE4C0, 0},
    { DnDirect, DnDirect, BWL, shiftReg, 0xE030, 0xE070, 0xE0B0},
    { Immediate, DnDirect, BWL, shiftReg, 0xE010, 0xE050, 0xE090}
};

flavor rtdfl[] = {
    { Immediate, 0, 0, immedWord, 0, 0x4E74, 0}
};

flavor rtefl[] = {
    { 0, 0, 0, zeroOp, 0, 0x4E73, 0}
};

flavor rtrfl[] = {
    { 0, 0, 0, zeroOp, 0, 0x4E77, 0}
};

flavor rtsfl[] = {
    { 0, 0, 0, zeroOp, 0, 0x4E75, 0}
};

flavor sbcdfl[] = {
    { DnDirect, DnDirect, BYTE, twoReg, 0x8100, 0x8100, 0},
    { AnIndPre, AnIndPre, BYTE, twoReg, 0x8108, 0x8108, 0}
};

flavor sccfl[] = {
    { DataAlt, 0, BYTE, scc, 0x54C0, 0x54C0, 0}
};

flavor scsfl[] = {
    { DataAlt, 0, BYTE, scc, 0x55C0, 0x55C0, 0}
};

flavor seqfl[] = {
    { DataAlt, 0, BYTE, scc, 0x57C0, 0x57C0, 0}
};

flavor sffl[] = {
    { DataAlt, 0, BYTE, scc, 0x51C0, 0x51C0, 0}
};

flavor sgefl[] = {
    { DataAlt, 0, BYTE, scc, 0x5CC0, 0x5CC0, 0}
};

flavor sgtfl[] = {
    { DataAlt, 0, BYTE, scc, 0x5EC0, 0x5EC0, 0}
};

flavor shifl[] = {
    { DataAlt, 0, BYTE, scc, 0x52C0, 0x52C0, 0}
};

flavor shsfl[] = {
    { DataAlt, 0, BYTE, scc, 0x54C0, 0x54C0, 0}
};

flavor slefl[] = {
    { DataAlt, 0, BYTE, scc, 0x5FC0, 0x5FC0, 0}
};

flavor slofl[] = {
    { DataAlt, 0, BYTE, scc, 0x55C0, 0x55C0, 0}
};

flavor slsfl[] = {
    { DataAlt, 0, BYTE, scc, 0x53C0, 0x53C0, 0}
};

flavor sltfl[] = {
    { DataAlt, 0, BYTE, scc, 0x5DC0, 0x5DC0, 0}
};

flavor smifl[] = {
    { DataAlt, 0, BYTE, scc, 0x5BC0, 0x5BC0, 0}
};

flavor snefl[] = {
    { DataAlt, 0, BYTE, scc, 0x56C0, 0x56C0, 0}
};

flavor splfl[] = {
    { DataAlt, 0, BYTE, scc, 0x5AC0, 0x5AC0, 0}
};

flavor stfl[] = {
    { DataAlt, 0, BYTE, scc, 0x50C0, 0x50C0, 0}
};

flavor stopfl[] = {
    { Immediate, 0, 0, immedWord, 0, 0x4E72, 0}
};

flavor subfl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0400, 0x0440, 0x0480},
    { Immediate, AnDirect, WL, quickMath, 0, 0x5140, 0x5180},
    { All, DnDirect, BWL, arithReg, 0x9000, 0x9040, 0x9080},
    { DnDirect, MemAlt, BWL, arithAddr, 0x9100, 0x9140, 0x9180},
    { All, AnDirect, WL, arithReg, 0, 0x90C0, 0x91C0},
};

flavor subafl[] = {
    { All, AnDirect, WL, arithReg, 0, 0x90C0, 0x91C0}
};

flavor subifl[] = {
    { Immediate, DataAlt, BWL, immedInst, 0x0400, 0x0440, 0x0480}
};

flavor subqfl[] = {
    { Immediate, DataAlt, BWL, quickMath, 0x5100, 0x5140, 0x5180},
    { Immediate, AnDirect, WL, quickMath, 0, 0x5140, 0x5180}
};

flavor subxfl[] = {
    { DnDirect, DnDirect, BWL, twoReg, 0x9100, 0x9140, 0x9180},
    { AnIndPre, AnIndPre, BWL, twoReg, 0x9108, 0x9148, 0x9188}
};

flavor svcfl[] = {
    { DataAlt, 0, BYTE, scc, 0x58C0, 0x58C0, 0}
};

flavor svsfl[] = {
    { DataAlt, 0, BYTE, scc, 0x59C0, 0x59C0, 0}
};

flavor swapfl[] = {
    { DnDirect, 0, WORD, oneReg, 0, 0x4840, 0}
};

flavor tasfl[] = {
    { DataAlt, 0, BYTE, oneOp, 0x4AC0, 0x4AC0, 0}
};

flavor trapfl[] = {
    { Immediate, 0, 0, trap, 0, 0x4E40, 0}
};

flavor trapvfl[] = {
    { 0, 0, 0, zeroOp, 0, 0x4E76, 0}
};

flavor tstfl[] = {
    { DataAlt, 0, BWL, oneOp, 0x4A00, 0x4A40, 0x4A80}
};

flavor unlkfl[] = {
    { AnDirect, 0, 0, oneReg, 0, 0x4E58, 0}
};


/* Define a macro to compute the length of a flavor list */

#define flavorCount(flavorArray) (sizeof(flavorArray)/sizeof(flavor))


/* The instruction table itself... */

instruction instTable[] = {
    { "ABCD", abcdfl, flavorCount(abcdfl), true, NULL},
    { "ADD",  addfl, flavorCount(addfl), true, NULL},
    { "ADDA", addafl, flavorCount(addafl), true, NULL},
    { "ADDI", addifl, flavorCount(addifl), true, NULL},
    { "ADDQ", addqfl, flavorCount(addqfl), true, NULL},
    { "ADDX", addxfl, flavorCount(addxfl), true, NULL},

    { "ALIGN", NULL, 0, false, AlignDirective},     // Align output

    { "AND", andfl, flavorCount(andfl), true, NULL},
    { "ANDI", andifl, flavorCount(andifl), true, NULL},

    { "APPL", NULL, 0, false, ApplDirective},  // Define app name and id

    { "ASL", aslfl, flavorCount(aslfl), true, NULL},
    { "ASR", asrfl, flavorCount(asrfl), true, NULL},
    { "BCC", bccfl, flavorCount(bccfl), true, NULL},
    { "BCHG", bchgfl, flavorCount(bchgfl), true, NULL},
    { "BCLR", bclrfl, flavorCount(bclrfl), true, NULL},
    { "BCS", bcsfl, flavorCount(bcsfl), true, NULL},

    { "BEGINPROC",  NULL, 0, false, BeginProcDirective}, // start procedure code

    { "BEQ", beqfl, flavorCount(beqfl), true, NULL},
    { "BGE", bgefl, flavorCount(bgefl), true, NULL},
    { "BGT", bgtfl, flavorCount(bgtfl), true, NULL},
    { "BHI", bhifl, flavorCount(bhifl), true, NULL},
    { "BHS", bccfl, flavorCount(bccfl), true, NULL},
    { "BLE", blefl, flavorCount(blefl), true, NULL},
    { "BLO", bcsfl, flavorCount(bcsfl), true, NULL},
    { "BLS", blsfl, flavorCount(blsfl), true, NULL},
    { "BLT", bltfl, flavorCount(bltfl), true, NULL},
    { "BMI", bmifl, flavorCount(bmifl), true, NULL},
    { "BNE", bnefl, flavorCount(bnefl), true, NULL},
    { "BPL", bplfl, flavorCount(bplfl), true, NULL},
    { "BRA", brafl, flavorCount(brafl), true, NULL},
    { "BSET", bsetfl, flavorCount(bsetfl), true, NULL},
    { "BSR", bsrfl, flavorCount(bsrfl), true, NULL},
    { "BTST", btstfl, flavorCount(btstfl), true, NULL},
    { "BVC", bvcfl, flavorCount(bvcfl), true, NULL},
    { "BVS", bvsfl, flavorCount(bvsfl), true, NULL},
    
    { "CALL",       NULL, 0, false, CallDirective}, // call procedure or trap
    
    { "CHK", chkfl, flavorCount(chkfl), true, NULL},
    { "CLR", clrfl, flavorCount(clrfl), true, NULL},
    { "CMP", cmpfl, flavorCount(cmpfl), true, NULL},
    { "CMPA", cmpafl, flavorCount(cmpafl), true, NULL},
    { "CMPI", cmpifl, flavorCount(cmpifl), true, NULL},
    { "CMPM", cmpmfl, flavorCount(cmpmfl), true, NULL},

    { "CODE", NULL, 0, false, CodeDirective},      // assemble to code segment
    { "DATA", NULL, 0, false, DataDirective},      // assemble to data segment

    { "DBCC", dbccfl, flavorCount(dbccfl), true, NULL},
    { "DBCS", dbcsfl, flavorCount(dbcsfl), true, NULL},
    { "DBEQ", dbeqfl, flavorCount(dbeqfl), true, NULL},
    { "DBF", dbffl, flavorCount(dbffl), true, NULL},
    { "DBGE", dbgefl, flavorCount(dbgefl), true, NULL},
    { "DBGT", dbgtfl, flavorCount(dbgtfl), true, NULL},
    { "DBHI", dbhifl, flavorCount(dbhifl), true, NULL},
    { "DBHS", dbccfl, flavorCount(dbccfl), true, NULL},
    { "DBLE", dblefl, flavorCount(dblefl), true, NULL},
    { "DBLO", dbcsfl, flavorCount(dbcsfl), true, NULL},
    { "DBLS", dblsfl, flavorCount(dblsfl), true, NULL},
    { "DBLT", dbltfl, flavorCount(dbltfl), true, NULL},
    { "DBMI", dbmifl, flavorCount(dbmifl), true, NULL},
    { "DBNE", dbnefl, flavorCount(dbnefl), true, NULL},
    { "DBPL", dbplfl, flavorCount(dbplfl), true, NULL},
    { "DBRA", dbrafl, flavorCount(dbrafl), true, NULL},
    { "DBT", dbtfl, flavorCount(dbtfl), true, NULL},
    { "DBVC", dbvcfl, flavorCount(dbvcfl), true, NULL},
    { "DBVS", dbvsfl, flavorCount(dbvsfl), true, NULL},

    { "DC", NULL, 0, false, dc},
    { "DCB", NULL, 0, false, dcb},

    { "DIVS", divsfl, flavorCount(divsfl), true, NULL},
    { "DIVU", divufl, flavorCount(divufl), true, NULL},

    { "DS", NULL, 0, false, ds},

    { "ELSE", NULL, 0, false, ElseDirective}, 		 // ELSE directive
    { "END",  NULL, 0, false, funct_end},
    
    { "ENDENUM",    NULL, 0, false, EndEnumDirective},   // end of enum type
    { "ENDIF",      NULL, 0, false, EndIfDirective}, 	 // ENDIF directive
    { "ENDPROC",    NULL, 0, false, EndProcDirective},   // end of procedure
    { "ENDPROXY",   NULL, 0, false, EndProxyDirective},  // end of proxy
    { "ENDSTRUCT",  NULL, 0, false, EndStructDirective}, // end of struct type
    { "ENDUNION",   NULL, 0, false, EndUnionDirective},  // end of union type
    { "ENUM",       NULL, 0, false, EnumDirective},      // start of enum type

    { "EOR", eorfl, flavorCount(eorfl), true, NULL},
    { "EORI", eorifl, flavorCount(eorifl), true, NULL},

    { "EQU", NULL, 0, false, equ},

    { "ERROR", NULL, 0, false, ErrorDirective},	 // programmer error message

    { "EXG", exgfl, flavorCount(exgfl), true, NULL},
    { "EXT", extfl, flavorCount(extfl), true, NULL},

    { "EXTERN", NULL, 0, false, ExternDirective}, // external variable
    { "GLOBAL", NULL, 0, false, GlobalDirective}, // global variable

    { "IF",     NULL, 0, false, IfDirective},     // IF directive
    { "IFDEF",  NULL, 0, false, IfDefDirective},  // IF directive
    { "IFNDEF", NULL, 0, false, IfNDefDirective}, // IF directive
    
    { "ILLEGAL", illegalfl, flavorCount(illegalfl), true, NULL},

    { "INCBIN",  NULL, 0, false, IncbinDirective},  // include a binary file
    { "INCLUDE", NULL, 0, false, IncludeDirective}, // include a file

    { "JMP", jmpfl, flavorCount(jmpfl), true, NULL},
    { "JSR", jsrfl, flavorCount(jsrfl), true, NULL},
    { "LEA", leafl, flavorCount(leafl), true, NULL},
    { "LINK", linkfl, flavorCount(linkfl), true, NULL},

    { "LIST", NULL, 0, false, ListDirective},   // turn output listing on/off
    { "LOCAL", NULL, 0, false, LocalDirective}, // local variable

    { "LSL", lslfl, flavorCount(lslfl), true, NULL},
    { "LSR", lsrfl, flavorCount(lsrfl), true, NULL},
    { "MOVE", movefl, flavorCount(movefl), true, NULL},
    { "MOVEA", moveafl, flavorCount(moveafl), true, NULL},
    { "MOVEC", movecfl, flavorCount(movecfl), true, NULL},
    { "MOVEM", NULL, 0, false, movem},
    { "MOVEP", movepfl, flavorCount(movepfl), true, NULL},
    { "MOVEQ", moveqfl, flavorCount(moveqfl), true, NULL},
    { "MOVES", movesfl, flavorCount(movesfl), true, NULL},
    { "MULS", mulsfl, flavorCount(mulsfl), true, NULL},
    { "MULU", mulufl, flavorCount(mulufl), true, NULL},
    { "NBCD", nbcdfl, flavorCount(nbcdfl), true, NULL},
    { "NEG", negfl, flavorCount(negfl), true, NULL},
    { "NEGX", negxfl, flavorCount(negxfl), true, NULL},
    { "NOP", nopfl, flavorCount(nopfl), true, NULL},
    { "NOT", notfl, flavorCount(notfl), true, NULL},
    { "OR", orfl, flavorCount(orfl), true, NULL},

#ifdef ORG_DIRECTIVE
    { "ORG", NULL, 0, false, org},
#endif

    { "ORI", orifl, flavorCount(orifl), true, NULL},
    { "PEA", peafl, flavorCount(peafl), true, NULL},

    { "PROC",       NULL, 0, false, ProcDirective},	// procedure definition
    { "PROCDEF",    NULL, 0, false, ProcDefDirective},	// procedure declaration
    { "PROXY",      NULL, 0, false, ProxyDirective},	// proxy definition

    { "REG", NULL, 0, false, reg},
    { "RES", NULL, 0, false, ResDirective}, // assemble to resource segment

    { "RESET", resetfl, flavorCount(resetfl), true, NULL},
    { "ROL", rolfl, flavorCount(rolfl), true, NULL},
    { "ROR", rorfl, flavorCount(rorfl), true, NULL},
    { "ROXL", roxlfl, flavorCount(roxlfl), true, NULL},
    { "ROXR", roxrfl, flavorCount(roxrfl), true, NULL},
    { "RTD", rtdfl, flavorCount(rtdfl), true, NULL},
    { "RTE", rtefl, flavorCount(rtefl), true, NULL},
    { "RTR", rtrfl, flavorCount(rtrfl), true, NULL},
    { "RTS", rtsfl, flavorCount(rtsfl), true, NULL},
    { "SBCD", sbcdfl, flavorCount(sbcdfl), true, NULL},
    { "SCC", sccfl, flavorCount(sccfl), true, NULL},
    { "SCS", scsfl, flavorCount(scsfl), true, NULL},
    { "SEQ", seqfl, flavorCount(seqfl), true, NULL},

    { "SET", NULL, 0, false, set}, // define re-defineable symbol

    { "SF", sffl, flavorCount(sffl), true, NULL},
    { "SGE", sgefl, flavorCount(sgefl), true, NULL},
    { "SGT", sgtfl, flavorCount(sgtfl), true, NULL},
    { "SHI", shifl, flavorCount(shifl), true, NULL},
    { "SHS", sccfl, flavorCount(sccfl), true, NULL},
    { "SLE", slefl, flavorCount(slefl), true, NULL},
    { "SLO", scsfl, flavorCount(scsfl), true, NULL},
    { "SLS", slsfl, flavorCount(slsfl), true, NULL},
    { "SLT", sltfl, flavorCount(sltfl), true, NULL},
    { "SMI", smifl, flavorCount(smifl), true, NULL},
    { "SNE", snefl, flavorCount(snefl), true, NULL},
    { "SPL", splfl, flavorCount(splfl), true, NULL},
    { "ST", stfl, flavorCount(stfl), true, NULL},
    { "STOP", stopfl, flavorCount(stopfl), true, NULL},

    { "STRUCT", NULL, 0, false, StructDirective}, // start of struct type

    { "SUB", subfl, flavorCount(subfl), true, NULL},
    { "SUBA", subafl, flavorCount(subafl), true, NULL},
    { "SUBI", subifl, flavorCount(subifl), true, NULL},
    { "SUBQ", subqfl, flavorCount(subqfl), true, NULL},
    { "SUBX", subxfl, flavorCount(subxfl), true, NULL},
    { "SVC", svcfl, flavorCount(svcfl), true, NULL},
    { "SVS", svsfl, flavorCount(svsfl), true, NULL},
    { "SWAP", swapfl, flavorCount(swapfl), true, NULL},

    { "SYSLIBTRAP", NULL, 0, false, CallDirective}, // call trap
    { "SYSTRAP",    NULL, 0, false, CallDirective}, // call trap

    { "TAS", tasfl, flavorCount(tasfl), true, NULL},
    { "TRAP", trapfl, flavorCount(trapfl), true, NULL},

    { "TRAPDEF",    NULL, 0, false, TrapDefDirective}, // trap definition

    { "TRAPV", trapvfl, flavorCount(trapvfl), true, NULL},
    { "TST", tstfl, flavorCount(tstfl), true, NULL},

    { "TYPEDEF",    NULL, 0, false, TypeDefDirective}, // type definition
    { "UNION",      NULL, 0, false, UnionDirective},   // start of union type

    { "UNLK", unlkfl, flavorCount(unlkfl), true, NULL}
};

/* Declare a global variable containing the size of the instruction table */

short int tableSize = sizeof(instTable)/sizeof(instruction);

