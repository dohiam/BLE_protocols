/*!
 * @file dbprint.cpp
 * @brief Support limited time printing of debug output
 * @details
 * This provides debug support that can't be provided simply through macros:
 * 1. Dynamically set debug level
 * 2. Printing debug output for a limited amount of time. 
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "dbprint.h"
#include <Arduino.h>

#ifdef DEBUG

static int db_lvl = DBLVL;
static unsigned long printfor = 0;
static unsigned long print_start_time;
static bool printed_end = false;
static unsigned long last_print_time = 0;

void DB_set_lvl(int lvl) { db_lvl = lvl; }
int  DB_get_lvl() { return db_lvl; }


unsigned long DB_delta() {
  unsigned long delta = millis() - last_print_time;
  last_print_time = millis();
  return delta;
}


static char sprintbuff[100];

char* DB_buffer() {
  return sprintbuff;
}

void DB_print_for(unsigned long t) {
  printfor = t; 
  print_start_time = millis(); 
  printed_end = false;
}

bool DB_time_expired() {
  if (printed_end) return true;
  if ((printfor == 0)) return true;
  if ((printfor + print_start_time) < millis()) {
    printed_end = true;
    PRINTF("===================== DEBUG OUTPUT ENDED ======================");
  }
  return false;
}

#endif
