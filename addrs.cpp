/*!
 * @file addrs.cpp
 * @brief Implementation of a simple database of devices found
 * @details
 * This currently is implemented using a fixed size array of db records. A todo is to provide something more flexible/efficient.
 * 
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Implementation of list to track MAC addresses.
 * 
 * Notes:
 * - TODO: something more flexible than fixed size array.

 * David Hamilton, 2021
 */
 
#include "addrs.h"
#include "dbprint.h"

static uint8_t num_addrs;

static tBDAddr addr_list[MAX_ADDRS];
static int connectables[MAX_ADDRS];
static int public_addrs[MAX_ADDRS];

void copy_addr(tBDAddr from, tBDAddr * to) {
  int i;
  for (i=0; i< 6; i++) (*to)[i] = from[i];
}

void zero_addr(tBDAddr * addr) {
  int i;
  for (i=0; i<6; i++) *addr[i] = 0;
}

bool addrs_match(tBDAddr addr1, tBDAddr addr2) {
  int i;
  for (i=0; i<6; i++) {
    if (addr1[i] != addr2[i]) return false; 
  }
  return true;
}

void print_addr(tBDAddr addr) {
  int i;
  for (i=5; i>0; i--) PRINTF("%02x:", addr[i]);
  PRINTF("%02X", addr[0]);
}

void init_addr_list() {
  num_addrs = 0;
}

// add to list if not already in the list
void add_addr(tBDAddr newaddr, bool connectable, bool public_addr) {
  bool found = 0;
  uint8_t addr, index;
  for (addr=0; addr<num_addrs && !found; addr++) {  
    for (index=0; index<6  && newaddr[index]==addr_list[addr][index]; index++);
    if (index==6) found = true;
  }
  if (!found) {
    for (index=0; index<6; index++) addr_list[num_addrs][index] = newaddr[index];
    if (connectable) connectables[num_addrs] = 1;
    else connectables[num_addrs] = 0; 
    if (public_addr) public_addrs[num_addrs] = 1;
    else public_addrs[num_addrs] = 0;
    num_addrs++;
  }
  else {
    if (connectable && (connectables[addr] == 0)) connectables[addr] = -1;
    if (public_addr && (public_addrs[addr] == 0)) public_addrs[addr] = -1;
  }
}

void print_addrs() {
  uint8_t addr,index;
  PRINTF("\n------------------- ADDR LIST ---------------------------\n");
  PRINTF("  CONNECTABLE        PUBLIC ADDR           ADDR\n");
  PRINTF(" -------------     ---------------     ------------------\n");
  for (addr=0; addr<num_addrs; addr++){
    switch (connectables[addr]) {
      case 1: 
        PRINTF("   CONNECTABLE      ");
        break;
      case 0:
        PRINTF(" NOT CONNECTABLE    ");
        break;
      case -1:
        PRINTF("       BOTH         ");
        break;
      default:
        PRINTF("         ?        ");
        break;
    };
    switch (public_addrs[addr]) {
      case 1: 
        PRINTF("      PUBLIC        ");
        break;
      case 0:
        PRINTF("   NOT PUBLIC       ");
        break;
      case -1:
        PRINTF("       BOTH         ");
        break;
      default:
        PRINTF("         ?        ");
        break;
    };
    for (index=5; index>0; index--) PRINTF("%02X:", addr_list[addr][index]);
    PRINTF("%02X\n", addr_list[addr][0]);
  }
  PRINTF("==================END OF ADDR LIST=======================\n"); 
}

static uint8_t next_addr;

void addr_enumeration_start() {
  next_addr = 0;
}

bool addr_enumeration_next(tBDAddr * addr_ptr, int * connectable, int * public_addr) {
  bool is_next = false;
  uint8_t addr, index;
  for (addr = next_addr; addr < num_addrs && !is_next; addr++) {
    next_addr = addr+1;
    is_next = true;
    for (index=0; index<8; index++) (*addr_ptr)[index] = addr_list[addr][index];
    *connectable = connectables[addr];
    *public_addr = public_addrs[addr];
  }
  return is_next;
}
