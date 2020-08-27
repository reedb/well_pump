// prc.c
//
// Routines for writing a Pilot PRC file
//
// Description:
// The functions in this file are used to write a Pilot PRC format
// executable (application) file.  In the absence of any official
// documentation on the PRC and executable image format I have been
// forced to make some guesses about the file structure based on what
// I've been able to discover from available PRC files.  Some of my
// guesses are probably flawed but until I can get my hands on some
// official documentation this is the best I can do.
//
// Author: Darrin Massena (darrin@massena.com)
// Date: 6/24/96
//
// Compression of datasection: Mikael Klasson (fluff@geocities.com)
// Date: 16 Jan 1998

#include "pila.h"
#include "prc.h"
#include "options.h"
#include "time.h"
#include "libiberty.h"

#ifndef unix
    //#include <windows.h>
    #include <winsock.h>
#else
    // for htonl and ntohl
    #include <asm/byteorder.h>
#endif

#ifdef unix
    typedef struct {
        unsigned long biSize;
        long int      biWidth;
        long int      biHeight;
        short int     biPlanes;
        short int     biBitCount;
        unsigned long biCompression;
        unsigned long biSizeImage;
        long int      biXPelsPerMeter;
        long int      biYPelsPerMeter;
        unsigned long biClrUsed;
        unsigned long biClrImportant;
    } BITMAPINFOHEADER;
    
    typedef struct {
        char rgbBlue;
        char rgbGreen;
        char rgbRed;
        char rgbReserved;
    } RGBQUAD;

    typedef struct {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD          bmiColors[1];
    } BITMAPINFO;
#endif

/////////////////////////////////////////////////////////////////////////////

extern int giPass;
extern long gcbResTotal;
long gcbDataCompressed;

#define kcbPrcMax   (512 * 1024)    // 512k should be plenty
#define kcrmeMax    1000            // 1000 resources should be enough

// #define offsetof(s,m)  (ulong)&(((s *)0)->m)

ResourceMapEntry garme[kcrmeMax];
long gcrme;
byte gpbPrc[kcbPrcMax];
long gcbPrc;
FourCC gfcCreatorId = MAKE4CC('T','E','M','P');

boolean CompressData(byte *pbData, ulong cbData, ulong cbUninitData,
                  byte **ppbCompData, ulong *pcbCompData);
boolean ConvertResource(ulong ulTypeOriginal, byte *pbResData,
                     ResourceMapEntry *prme);

/////////////////////////////////////////////////////////////////////////////

boolean AddResource(FourCC fcType, ushort usId, byte *pbData, ulong cbData,
                 boolean fHead)
{
    ResourceMapEntry *prme;

    if (giPass<2) {
        return true;
    }

    if (fHead) {
        prme = garme;
        memmove(prme + 1, prme, sizeof(ResourceMapEntry) * gcrme);
    } else {
        prme = &garme[gcrme];
    }

    prme->fcType = fcType;
    prme->usId = usId;
    //  prme->pbData = pbData;
    prme->cbData = cbData;
    gcrme++;

    if (OPTION(resources_only)) {
        prme->pbData = xmalloc(cbData);
        memcpy(prme->pbData, pbData, cbData);
    } else {
        if (!ConvertResource(fcType, pbData, prme)) {
            return false;
        }
    }

    gcbResTotal += cbData + 10;     // 10 bytes = resource map overhead

    return true;
}

/////////////////////////////////////////////////////////////////////////////


// cbData = cbA + cbB;
// a5 = new byte[cbData] + cbB;
// *a5 = SysAppInfoPtr

struct CodeZeroResource {
    ulong cbA;      // Initialized data size
    ulong cbB;      // Uninitialized data size
} czr;

long WritePrc(char *pszFileName, char *pszAppName, byte *pbCode, long cbCode,
              byte *pbData, long cbData)
{
    extern boolean LayoutPrc(char *pszFilename, char *pszAppName);
    boolean fSuccess;
    FILE *pfilOut;
    char szName[_MAX_PATH];
    long cbRes;

    if (!OPTION(resources_only)) {
        //
        // Add resources for code, data info, and data.
        //

        if (cbData) {
            AddResource(MAKE4CC('d','a','t','a'), 0x0000, pbData, cbData, true);
        }

        // Write the mystery 'code' 0000 resource.
        // FIX:
        czr.cbA = htonl(cbData);
        czr.cbB = htonl(0);
        AddResource(MAKE4CC('c','o','d','e'), 0x0000, (byte *)&czr, sizeof(czr), true);

        // Write the real 'code' resource.
        AddResource(MAKE4CC('c','o','d','e'), 0x0001, pbCode, cbCode, true);
    }

    // Create file for writing.
    pfilOut = fopen(pszFileName, "wb");
    if (pfilOut == NULL || pfilOut == (FILE *)-1) {
        printf("Error: Can't open output file \"%s\"\n", pszFileName);
        return 0;
    }

    if ( ( !pszAppName ) || ( !*pszAppName ) ) {
#ifdef unix
        pszAppName = strrchr(szName, '/');
        if (!pszAppName) {
            pszAppName = szName;
        } else {
            pszAppName++;
        }
#else
        _splitpath(pszFileName, NULL, NULL, szName, NULL);
        pszAppName = szName;
#endif
    }

    if (strlen(pszAppName) > 31) {
        printf("Error: Application name %s is too long. 31 characters max.\n",
               pszAppName);
        return 0;
    }

    fSuccess = LayoutPrc(pszFileName, pszAppName);
    if (fSuccess) {
        fSuccess = fwrite(gpbPrc, gcbPrc, 1, pfilOut) == 1;
    }

    cbRes = ftell(pfilOut);
    fclose(pfilOut);

    return cbRes;
}


/////////////////////////////////////////////////////////////////////////////

boolean LayoutPrc(char *pszFilename, char *pszAppName)
{
    byte *pbResData;
    ResourceMapEntry *prme;
    DatabaseHdrType *dbHdr = (DatabaseHdrType *)&gpbPrc;
    RsrcEntryType *resEntry;
    int i;

    //
    // Here we go!
    //

    // Initialize the output buffer to all zeros.
    memset(gpbPrc, 0, kcbPrcMax);

    // Sneak "Pila" into the (most likely) unused app name space.
    *((ulong *)(&(dbHdr->name[28]))) = ntohl(MAKE4CC('P','i','l','a'));

    // The first 32 bytes of a PRC is the filename.
    strcpy(dbHdr->name, pszAppName);

    dbHdr->attributes = htons(dmHdrAttrResDB|dmHdrAttrBackup|dmHdrAttrBundle);
    dbHdr->version    = htons(1);
    dbHdr->creationDate = ntohl(time(0L)+2082844800);
    dbHdr->modificationDate = ntohl(time(0L)+2082844800);
    dbHdr->lastBackupDate = ntohl(0);
    dbHdr->modificationNumber = ntohl(0);
    dbHdr->appInfoID = ntohl(0);
    dbHdr->sortInfoID = ntohl(0);
    dbHdr->type = ntohl(MAKE4CC_FROM_STRING(OPTION(database_type)));
    dbHdr->creator = ntohl(gfcCreatorId);
    dbHdr->uniqueIDSeed = ntohl(123456);

    dbHdr->recordList.nextRecordListID = ntohl(0);
    dbHdr->recordList.numRecords = htons(gcrme);

    resEntry  = &(dbHdr->recordList.firstEntry); // address of first entry in resource map
    pbResData = (byte *)(&(resEntry[gcrme]));     // offset of first resource data-2

    *pbResData++ = 0; // two null bytes following the resource map
    *pbResData++ = 0;

    prme = garme;
    for (i = 0; i < gcrme; i++, prme++)
    {
        // Copy the resource type to the resource map
        resEntry->type = ntohl(prme->fcType);

        // Copy the resource id to the resource map
        resEntry->id = htons(prme->usId);

        // Copy the offset to the resource data in the resource map
        resEntry->localChunkID = htonl(pbResData - gpbPrc);

        // Copy the resource data to the appropriate offset
        memcpy(pbResData, prme->pbData, prme->cbData);

        // Point to the next available resource map and data destination.
        resEntry++;
        pbResData += prme->cbData;
    }

    gcbPrc = pbResData - gpbPrc;

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// This information was sent to me by Steve Lemke (a Palm Computing
// developer).
//
// Here's the structure of a Metrowerks DATA 0 resource:
//
// +---------------------------------+
// | long:   offset of CODE 1 xrefs  |---+
// +---------------------------------+   |
// | char[]: compressed init data    |   |
// +---------------------------------+   |
// | char[]: compressed DATA 0 xrefs |   |
// +---------------------------------+   |
// | char[]: compressed CODE 1 xrefs |<--+
// +---------------------------------+

boolean CompressData(byte *pbData, ulong cbData, ulong cbUninitData,
                  byte **ppbCompData, ulong *pcbCompData)
{
    unsigned int i;
    unsigned char c;
    long lOffset;

    // Allocate a temporary compression buffer.

    byte *pbCompBuffer = (byte *)xmalloc(
            (cbData * 2) + (3 * 5) + (6 * sizeof(ulong)) + 40);   // +40 is just in case
    byte *pbComp = pbCompBuffer;

    // The first ulong in a 'data' resource is an offset to the compressed
    // CODE 1 xrefs. In PalmOS 1.0 it appears to be unused by the loader.

    *(ulong *)pbComp = htonl(cbData);
    pbComp += 4;

    //
    // NOTE: Read this fascinating PalmOS tidbit and corresponding workaround
    //
    // After the PalmOS loads an application the application startup code calls
    // it back (SysAppStartup) to allocate dynamic memory for its data section,
    // point A5 to the dynamic memory, then decompress the stored data into it.
    // After allocating and initializing A5 but before decompressing
    // SysAppStartup stuffs a pointer to the app's SysAppInfo structure at the
    // location pointed to by A5.
    //
    // This is a problem because the Pila data section decompresses at the
    // address pointed to by A5, overwriting the SysAppInfo pointer. Apparently
    // PalmOS APIs (at least the Frm* APIs) need the SysAppInfo pointer and
    // look for it hanging off A5 so the nasty side-effect of this collision
    // are some interesting (and somewhat random, depending on what data
    // values are decompressed) crashes.
    //
    // The fix is to force decompression to start at A5+4 (skipping the
    // SysAppInfo pointer). No problem, the PalmOS DecompressData routine has
    // a facility for such things. We can't just change move the data by
    // 4 bytes though because the assembled code assumes the data is based
    // at A5. So we include a unused dummy long of data in the Startup.inc
    // code. Assuming the first long is unused, this code skips it and sets
    // things up so the rest of the data will be decompressed from there.
    //

    lOffset = 4;    // skip first 4 bytes where SysAppInfo* will be stored
    cbData -= 4;
    pbData += 4;

    // The second ulong in a 'data' resource is the offset from A5
    // (positive or negative) that the data should be stored at.

    *(ulong *)pbComp = htonl(lOffset);
    pbComp += 4;

    // Compress the data (only parts of the pilot's enhanced RLE implemented).

    while (cbData > 0) {
        if (cbData > 1) {
            c = *pbData;
            if (!c) {
                for (i = 0; ( i < cbData ) && ( i < 0x40 ) && (*(pbData + i) == c); i++) {
                    ;
                }
                if( i > 0 ) {
                    *pbComp++ = 0x40 + i - 1;
                    pbData += i;
                    cbData -= i;
                    continue;
                }
            } else if (c == 0xff) {
                for (i = 0; i < cbData && i < 0x10 && (*(pbData + i) == c); i++) {
                    ;
                }
                if( i > 0 ) {
                    *pbComp++ = 0x10 + i - 1;
                    pbData += i;
                    cbData -= i;
                    continue;
                }
            } else {
                for (i = 0; i < cbData && i < 0x20 && (*(pbData + i) == c); i++) {
                    ;
                }
                if( i > 1 ) {
                    *pbComp++ = 0x20 + i - 2;
                    *pbComp++ = c;
                    pbData += i;
                    cbData -= i;
                    continue;
                }
            }
        }
        for (i = 0; i < cbData && i < 0x80 && (*(pbData + i + 1) != *(pbData + i)); i++) {
            ;
        }
        if (!i) {
            i = 1;
        }
        *pbComp++ = 0x80 + i - 1;
        memcpy(pbComp, pbData, i);
        pbComp += i;
        pbData += i;
        cbData -= i;
    }

    // The decompressor expects 3 groups of { a5offset, compressed stream }
    // separated by a 0 byte. This code fills out the remainder.

    *pbComp++ = 0;

    *(ulong *)pbComp = 0;
    pbComp += sizeof(ulong);
    *pbComp++ = 0;

    *(ulong *)pbComp = 0;
    pbComp += sizeof(ulong);
    *pbComp++ = 0;

    // An additional six ulongs of zero

// From Steve Lemke (Palm):
// "However, this DATA 0 resource SHOULD contain an additional SIX
// longwords each set to zero to specify that each of the sub-blocks of
// relocation data (three sub-blocks in the DATA 0 xrefs section and three
// sub-blocks in the CODE 1 xref section) contains no xrefs.  The first
// longword in each of the six sub-blocks is a count of that sub-block's xrefs."

    memset(pbComp, 0, 6 * sizeof(ulong));
    pbComp += 6 * sizeof(ulong);

    // Copy the compressed results to a right-sized buffer.

    *pcbCompData = pbComp - pbCompBuffer;
    *ppbCompData = (byte *)xmalloc(*pcbCompData);
    memcpy(*ppbCompData, pbCompBuffer, *pcbCompData);

    // Free temp compression buffer.

    free(pbCompBuffer);

    gcbDataCompressed = pbComp - pbCompBuffer;

    return true;
}

/////////////////////////////////////////////////////////////////////////////

// Test bit in a 1bpp bitmap

boolean TestBit(int cx, int cy, byte *pb, int x, int y, int cBitsAlign)
{
    int cbRow = (cx + (cBitsAlign - 1)) & ~(cBitsAlign - 1);
    pb += (cbRow >> 3) * y + (x >> 3);

    return (*pb & (1 << (7 - (x & 7))));
}

// Set bit in a 1bpp bitmap

void SetBit(int cx, int cy, byte *pb, int x, int y, int cBitsAlign)
{
    int cbRow = (cx + (cBitsAlign - 1)) & ~(cBitsAlign - 1);
    pb += (cbRow >> 3) * y + (x >> 3);
    *pb |= (1 << (7 - (x & 7)));
}

/////////////////////////////////////////////////////////////////////////////

// Pilot bitmap resource format

typedef struct _Bitmap { // bm
    ushort cx;
    ushort cy;
    ushort cbRow;
    ushort ff;
    ushort ausUnk[4];
    byte abBits[1];
} Bitmap;


boolean ConvertBitmapResource(byte *pbResData, ResourceMapEntry *prme)
{
    byte *pbSrc;
    int cbDst, cbRow;
    Bitmap *pbmDst;
    int x, y;

#ifndef unix
    BITMAPINFO *pbmi = (BITMAPINFO *)(pbResData + sizeof(BITMAPFILEHEADER));
#else
    BITMAPINFO *pbmi = (BITMAPINFO *)(pbResData + 14);
#endif
    pbSrc = ((byte *)pbmi) + pbmi->bmiHeader.biSize + sizeof(RGBQUAD) * 2;

    // If image not 1bpp, bail

    if (pbmi->bmiHeader.biBitCount != 1) {
        printf("Bitmap not 1 bpp!");
        return false;
    }

    // Alloc what we need for image data
    // Pilot images are word aligned

    cbRow = ((pbmi->bmiHeader.biWidth + 15) & ~15) / 8;
    cbDst = cbRow * pbmi->bmiHeader.biHeight + offsetof(Bitmap, abBits);
    pbmDst = (Bitmap *)xmalloc(cbDst);

    // Image data has been inverted for Macintosh, so invert back

    memset(pbmDst, 0, cbDst);

    // Convert from source bitmap format (dword aligned) to dst format (word
    // aligned).

    for (y = 0; y < pbmi->bmiHeader.biHeight; y++) {
        for (x = 0; x < pbmi->bmiHeader.biWidth; x++) {
            // Reverse bits so we get WYSIWYG in msdev (white
            // pixels become background (0), black pixels become
            // foreground (1)).

            int yT = y;
            if (pbmi->bmiHeader.biHeight > 0) {
                yT = pbmi->bmiHeader.biHeight - y - 1;
            }

            if (!TestBit(pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight,
                         pbSrc, x, yT, 32)) {
                SetBit(pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight,
                       &pbmDst->abBits[0], x, y, 16);
            }
        }
    }
    pbmDst->cx = htons((ushort)pbmi->bmiHeader.biWidth);
    pbmDst->cy = htons((ushort)pbmi->bmiHeader.biHeight);
    pbmDst->cbRow = htons((ushort)cbRow);

    // Update resource entry with new pos / size

    prme->pbData = (byte *)pbmDst;
    prme->cbData = cbDst;

    // Special case: if bitmap id is 7FFE, make it the app
    // icon

    if (prme->usId == 0x7FFE) {
        prme->fcType = MAKE4CC('t','A','I','B');
        prme->usId = 1000;

        // Make sure it's the right size / height

        if (abs(pbmi->bmiHeader.biHeight) !=32 || pbmi->bmiHeader.biWidth != 32) {
            printf("Icon resource not 32x32!\n");
            return false;
        }
    } else {
        prme->fcType = MAKE4CC('T','b','m','p');
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// - Compresses 'data' resource
// - Converts 'WBMP' resources to either 'tBMP' or 'tAIB'

boolean ConvertResource(ulong ulTypeOriginal, byte *pbResData,
                     ResourceMapEntry *prme)
{
    switch (ulTypeOriginal) {
    case MAKE4CC('d','a','t','a'):
            // Compress the 'data' data before adding it.

        CompressData(pbResData, prme->cbData, 0, &prme->pbData,
                     &prme->cbData);
        break;

    case MAKE4CC('W','B','M','P'):
        if (!ConvertBitmapResource(pbResData, prme)) {
            return false;
        }
        break;

    default:
        // No conversion, just copy

        prme->pbData = xmalloc(prme->cbData);
        memcpy(prme->pbData, pbResData, prme->cbData);
        break;
    }

    return true;
}

