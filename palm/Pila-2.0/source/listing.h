/***********************************************************************
 *
 *      LISTING.H
 *      Listing File Routines for 68000 Assembler
 *
 *    ListInitialize(char *listFileName)
 *      Opens the specified listing file for writing. If the
 *      file cannot be opened, then the routine prints a
 *      message and exits.
 *
 *    ListClose(char *szErrors)
 *      Closing of the listing file after writing the error statistics
 *      passed in with szErrors.
 *
 *    ListStartListing()
 *      There will be no listing output (even after calls to
 *      ListInitialize and ListEnable) until this function has been
 *      called. That way any listing output can be deferred until
 *      the last pass is started.
 *
 *    ListEnable()
 *      Enables writing to the listing file.
 *
 *    ListDisable()
 *      Disables writing to the listing file.
 *
 *    ListWriteLine()
 *      Writes the current listing line to the listing file.
 *      If a current source line is available it is appended to
 *      the listing data that was set through the ListPut... routines.
 *      If the current source line is not an expansion-line the current
 *      source file line number is written as well.
 *      If an error occurs during the writing, the routine prints a 
 *      message and stops the program.
 *
 *    ListWriteError(char *errorMsg)
 *      Adds an error message to the listing file. The actual writing of
 *      the error message is deferred until after the offending source
 *      line has been written to the listing file.
 *      The string passed in as parameter must have been allocated via
 *      malloc. If there is no listing file written the function returns
 *      true to signal to the caller that the message has not been stored
 *      (and can be freed) otherwise false is returned and the message will
 *      be freed by ListWriteLine later on.
 *
 *    ListPutSourceLine(char *sourceLine, int sourceLineNo)
 *      This call stores the source line and the current line no for
 *      later inclusion in the line written to the listing file.
 *
 *    ListPutLocation(unsigned long outputLocation)
 *      Starts the process of assembling a listing line by
 *      printing the location counter value into listData and
 *      initializing listPtr.
 *
 *    ListPutSymbol(long data)
 *      This call prints the value of a symbol into the object field
 *      of the listing line. It is called by the directives EQU and SET.
 *
 *    ListPutData(long data,int size)
 *      Prints the data whose size and value are specified in
 *      the object field of the current listing line. Bytes are
 *      printed as two digits, words as four digits, and
 *      longwords as eight digits, and a space follows each
 *      group of digits.
 *           If the item to be printed will not fit in the
 *      object code field, one of two things will occur. If option
 *      const_expanded is TRUE, then the current listing line will
 *      be output to the listing file and the data will be printed
 *      on a continuation line. Otherwise, elipses ("...") will
 *      be printed to indicate the omission of values from the
 *      listing, and the data will not be added to the file.
 *
 *      Author: Frank Schaeckermann
 *
 *        Date: 2003-08-14
 *
 *      Change Log:
 *
 *
 ************************************************************************/

#ifndef _LISTING_H_
#define _LISTING_H_

#include "pila.h"

void ListInitialize(char *listFileName);
void ListClose(char *szErrors);
void ListStartListing();
void ListEnable();
void ListDisable();
boolean ListWriteError(char *errorMsg);
void ListWriteLine();
void ListPutSourceLine(char *sourceLine,int sourceLineNo);
void ListPutLocation(unsigned long outputLocation);
void ListPutSymbol(long data);
void ListPutTypeName(char *name);
void ListPutData(long data, int size);

#endif
