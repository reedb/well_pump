// PRC.h
//
// Data structures, constants, macros and prototypes used by PRC.c and the
// callers of the functions it contains.
//
// Author: Darrin Massena (darrin@massena.com)
// Date: 6/24/96

#ifndef __PRC_H__
#define __PRC_H__

#include "pila.h"
#include "asm.h"

#ifndef	__USE_MISC			/* FSCHAECK - removed */
//	typedef unsigned short ushort;  // us
	typedef unsigned long ulong;    // ul
#endif					/* FSCHAECK - removed */

typedef unsigned char byte;     // b
typedef ulong FourCC;           // fc
#define MAKE4CC(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | ((d) << 0))
#define MAKE4CC_FROM_STRING(s) MAKE4CC(s[0],s[1],s[2],s[3])

#ifndef NULL
    #define NULL    0
#endif

#define SwapWords(ul)   (ulong)((((ulong)(ul)) >> 16) | (((ulong)(ul)) << 16))
#define SwapBytes(us)   (ushort)((((ushort)(us)) >> 8) | (((ushort)(us)) << 8))
#define SwapLong(ul)    SwapWords(ul)
#define SwapShort(us)   SwapBytes(us)
#define ReverseLong(ul) (ulong)((((ulong)(ul)) >> 24) | (((ulong)(ul)) << 24) | ((((ulong)(ul)) & 0x00FF0000) >> 8) | ((ul & 0x0000FF00) << 8))

// Palm database specific definitions (PRC)

#define dmDBNameLength 32

#define	dmHdrAttrResDB		   0x0001	// Resource database
#define dmHdrAttrReadOnly	   0x0002	// Read Only database
#define	dmHdrAttrAppInfoDirty	   0x0004	// Set if Application Info block is dirty
					        // Optionally supported by an App's conduit
#define	dmHdrAttrBackup		   0x0008	// Set if database should be backed up to PC if
					        // no app-specific synchronization conduit
                                                // has been supplied.
#define	dmHdrAttrOKToInstallNewer  0x0010	// This tells the backup conduit that it's OK
						//  for it to install a newer version of this database
						//  with a different name if the current database is
						//  open. This mechanism is used to update the 
						//  Graffiti Shortcuts database, for example. 
#define	dmHdrAttrResetAfterInstall 0x0020 	// Device requires a reset after this database is 
						// installed.
#define	dmHdrAttrCopyPrevention	   0x0040	// This database should not be copied to 
#define	dmHdrAttrStream		   0x0080	// This database is used for file stream implementation.
#define	dmHdrAttrHidden		   0x0100	// This database should generally be hidden from view
						//  used to hide some apps from the main view of the
						//  launcher for example.
						// For data (non-resource) databases, this hides the record
						//	 count within the launcher info screen.
#define	dmHdrAttrLaunchableData    0x0200	// This data database (not applicable for executables)
					        //  can be "launched" by passing it's name to it's owner
					        //  app ('appl' database with same creator) using
					        //  the sysAppLaunchCmdOpenNamedDB action code. 
#define	dmHdrAttrRecyclable        0x0400	// This database (resource or record) is recyclable:
					        //  it will be deleted Real Soon Now, generally the next
					        //  time the database is closed. 
#define	dmHdrAttrBundle	           0x0800	// This database (resource or record) is associated with
					        // the application with the same creator. It will be beamed
					        // and copied along with the application. 
#define	dmHdrAttrOpen              0x8000	// Database not closed properly


#pragma pack(push,2)

typedef ulong LocalID;

typedef struct {
  ulong type;
  ushort id;
  LocalID localChunkID;
} RsrcEntryType;

typedef struct {
  LocalID nextRecordListID;
  ushort numRecords;
  RsrcEntryType firstEntry;
} RecordListType;

typedef struct {
  byte    name[dmDBNameLength];
  ushort  attributes;
  ushort  version;
  ulong   creationDate;
  ulong   modificationDate;
  ulong   lastBackupDate;
  ulong   modificationNumber;
  LocalID appInfoID;
  LocalID sortInfoID;
  ulong   type;
  ulong   creator;
  ulong   uniqueIDSeed;
  RecordListType recordList;
} DatabaseHdrType;

#pragma pack(pop)


typedef struct _ResourceMapEntry { // rme
    FourCC fcType;
    ushort usId;
    byte *pbData;
    ulong cbData;
} ResourceMapEntry;

boolean AddResource(FourCC fcType, ushort usId, byte *pbData, ulong cbData,
                 boolean fHead);
long WritePrc(char *pszFileName, char *pszAppName, byte *pbCode, long cbCode,
              byte *pbData, long cbData);

#endif // ndef __PRC_H__
