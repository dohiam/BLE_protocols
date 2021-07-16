/*!
 * @file dbprint.h
 * @brief A collection of functions to ease the use of debugging statements. 
 * @details
 * There are several levels of debugging supported; everything at or below the defined level is printed.
 * Nothing is printed if the DEBUG flag is not set.
 * 
 * Note that too much debugging output can cause LOST EVENTS where the Arduino can't process messages fast enough and the BLUENRG event queue overflows.
 * Look for lost events flagged in the debug output to see if this is happening. A good practice is to increase debugging at times of interest, and then
 * dial it back down. Another good practice is to limit the amount of time debug output is produced. If things go haywire and your program spews debug 
 * output constantly and indefinitely, the Arduino programmer can't talk to the Arduino and you would need to do a manual reset to recover. Limiting the
 * time debug output is printed can help avoid this problem.
 * 
 * Summary of macros as examples (note that they should not be ended with a semicolon and not with a \n in the string as that is always included automatically):
 *  - DB_print_for(FIVE_MINUTES) to set up debugging printout for 30,000,000 milliseconds
 *  - DBPR(debug_level, variable_name, "%d", "example of printing variable_name as a decimal number but print it however you like using your own format string")
 *  - DBBUFF(debug_level, variable_name) to print the first few bytes of variable_name as a hex string which will generally take up a complete line of output
 *  - DBSTR(debug_level, variable_name) is the same as above except that it is printed as a string instead of in hex
 *  - DBMSG(debug_level, "any string you like that will be printed with a \n at the end so you don't need to include it in your string")
 *  - DBADDR(debug_level, addr_variable_name, "a 6 byte address, i.e., tBDAddr type) and a message like this")
 *  - DBPRN(debug_level, VARaddr_variable_name, number_of_bytes, "print a variable of a given size as hex string plus message")
 *  - DBPRNS(debug_level, VARaddr_variable_name, number_of_bytes, "print a variable of a given size as character string plus message")
 * 
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#ifndef DBPRINT_H
#define DBPRINT_H

#define DEBUG 1
#define DBLVL 3

#include <stdbool.h>

// Debug levels for observation
#define DBL_RAW_EVENT_DATA     8
#define DBL_HAL_EVENTS         7
#define DBL_HCI_EVENTS         6
#define DBL_ALL_BLE_EVENTS     5
#define DBL_DECODED_EVENTS     4
#define DBL_IMPORTANT_EVENTS   3
#define DBL_WARNINGS           2
#define DBL_ERRORS             1
#define DBL_NADA               0

// Debug print output macros and functions.
// Facilitate debugging/tracing using print statements over the serial port.

// Limit printing because print volume can get large quickly and hog the serial port, especially
// during high event times like observation and general_discovery. You can set the initial level
// (DBLVL low) and then temporary increase it (DB_set_lvl(n)) when you want extra debug output.
//
// Trigger debug output to start or resume anytime by calling DB_start and
// telling it how many ms you want to enable printing for.
// If you do not call this, debug out will spew forever and could make it difficult to reprogram.

// First, this all provides quite a bit of overhead so it needs to be compiled out when not really needed

#ifndef DEBUG
void DB_set_lvl(int lvl) {}
int  DB_get_lvl() {return 0;}
void DB_print_for(unsigned long t) {} 
unsigned long DB_delta {}
#define DB_BREADCRUMB {}
#define DBPR(DBNUM, DBVAR, DBFMT, DBMSG)  {}
#define DBBUFF(DBNUM,DBVAR) {}
#define DBSTR(DBNUM,DBVAR) {}
#define DBMSG(DBNUM, MSG) {}
#define DBADDR(DBNUM, ADDR, MSG) {}
#define PRINTF(...) {}
#define CASE_PRINT_ENUM(E) case E: break;
#else

void DB_set_lvl(int lvl);
int  DB_get_lvl();

unsigned long DB_delta();

bool DB_time_expired();

void DB_print_for(unsigned long t);

#define DBLIMIT_BEGIN(DBNUM) \
if ( !DB_time_expired() ) {  \
   if (DB_get_lvl() >= DBNUM) { 
    
#define DBLIMIT_END \  
   }  \ 
}

// Limit printing by setting the debug level.
// Anything less than or equal to this will print.
#ifndef DBLVL
#define DBLVL 9
#endif

char* DB_buffer();

// A general print statement, not limited.
#define PRINTF(...) {sprintf(DB_buffer() ,__VA_ARGS__); SerialUSB.print(DB_buffer());}

// The following is a macro trick to enable strings passed in to be printed as part of the output
#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define DB_BREADCRUMB PRINTF("At %s in %s\n", __FILE__, __LINE__)

// DBPR: Good for printing out one variable value along with a message about the context and/or what it means 
#define DBPR(DBNUM, DBVAR, DBFMT, DBMSG)  \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) "STRINGIFY(DBVAR)"=", millis(), DB_delta()); \
  PRINTF(DBFMT, DBVAR); \
  PRINTF(" (");  \
  PRINTF(DBMSG); \
  PRINTF(")\n"); \
DBLIMIT_END
 
//The following is good for printing out buffers handed down by BlueRNG events
#define DBBUFF(DBNUM,DBVAR) \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) "STRINGIFY(DBVAR)": ", millis(), DB_delta()); \
  for (int temp=0; temp<40; temp++) { \
    PRINTF("%02X:", ((uint8_t *) (DBVAR))[temp]); \
  } \
  PRINTF("\n"); \
DBLIMIT_END

//Similar to above but print as chars instead of hex
#define DBSTR(DBNUM,DBVAR) \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) "STRINGIFY(DBVAR)": ", millis(), DB_delta()); \
  for (int temp=0; temp<40; temp++) { \
    PRINTF("%c", DBVAR[temp]); \
  } \
  PRINTF("\n"); \
DBLIMIT_END

//The following is to just display a (timestamped) message
#define DBMSG(DBNUM, MSG) \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) ", millis(), DB_delta());  \
  PRINTF(MSG); \
  PRINTF("\n"); \
DBLIMIT_END

#define DBADDR(DBNUM, ADDR, MSG) \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) ", millis(), DB_delta());  \
  PRINTF(MSG); \
  PRINTF(" "STRINGIFY(ADDR)" Address = "); \
  for (uint8_t TeMpInDeX=5; TeMpInDeX>0; TeMpInDeX--) { \
    PRINTF("%02X", ADDR[TeMpInDeX]); \
    PRINTF(":"); \
  } \
  PRINTF("%02X\n", ADDR[0]); \
DBLIMIT_END

#define DBPR8(DBNUM, BYTE8, MSG) \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) ", millis(), DB_delta());  \
  PRINTF(MSG); \
  PRINTF(" "STRINGIFY(BYTE8)" 8 Bytes = "); \
  for (uint8_t TeMpInDeX=0; TeMpInDeX<8; TeMpInDeX++) { \
    PRINTF("%02X", BYTE8[TeMpInDeX]); \
    if (TeMpInDeX ==7) PRINTF("\n")  \
    else PRINTF(":"); \
  } \
DBLIMIT_END

#define DBPRN(DBNUM, VAR, SIZE, MSG) \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) ", millis(), DB_delta());  \
  PRINTF(MSG); \
  PRINTF(" "STRINGIFY(VAR)" size: %d, Bytes = ", SIZE); \
  for (uint8_t TeMpInDeX=0; TeMpInDeX<SIZE; TeMpInDeX++) { \
    PRINTF("%02X", VAR[TeMpInDeX]); \
    if (TeMpInDeX ==(SIZE-1)) PRINTF("\n")  \
    else PRINTF(":"); \
  } \
DBLIMIT_END

#define DBPRNS(DBNUM, VAR, SIZE, MSG) \
DBLIMIT_BEGIN(DBNUM) \ 
  PRINTF("DBUG %-8d (%-3d) ", millis(), DB_delta());  \
  PRINTF(MSG); \
  PRINTF(" "STRINGIFY(VAR)" size: %d, Bytes = ", SIZE); \
  for (uint8_t TeMpInDeX=0; TeMpInDeX<SIZE; TeMpInDeX++) { \
    PRINTF("%c", VAR[TeMpInDeX]); \
    if (TeMpInDeX ==(SIZE-1)) PRINTF("\n")  \
  } \
DBLIMIT_END

#define CASE_PRINT_ENUM(x) case x: DBMSG(DBL_DECODED_EVENTS, #x) break;

#endif
#endif
