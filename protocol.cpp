/*!
 * @file protocol.cpp
 * @brief provides implementations of the functions to set, get, clear, and run the current protocol
 * @details
 * The primary function is run_current_protocol which should be called by the BlueRNG callback function. That is, somewhere in your code, like the main sketch,
 * should be the following:
 * 
 *     void HCI_Event_CB(void *pckt) {
 *       run_current_protocol(pckt);
 *     }
 *     
 * There is only one protocol allowed to be running at a time (captured in a static variable used by the framework).
 * 
 * The remaining functions are supporting functions by the framework, used automatically by the macros in production.h
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "protocol.h"
#include "production.h"
#include "dbprint.h"

static protocol_ptr_t current_protocol;

void run_current_protocol(void *pckt) {
  int production_result;
  bool protocol_is_working;
  protocol_ptr_t current_protocol;
  DBMSG(DBL_DECODED_EVENTS, "----------------------------------------------------------")
  DBBUFF(DBL_RAW_EVENT_DATA, pckt)
  production_result = run_production(pckt);
  switch (production_result) {
    case 0:  
      DBMSG(DBL_ALL_BLE_EVENTS, "current production finished")  
      current_protocol = get_current_protocol();
      if (current_protocol) {
        protocol_is_working = (*current_protocol)();
        if (!protocol_is_working) {
          PRINTF("current protocol encountered an error - clearing current protocol\n")
          clear_current_protocol();
        }
      }
      else PRINTF("no current protocol to call\n");
      break;
    case -1:
      DBMSG(DBL_ALL_BLE_EVENTS, "current production did not run any rules")  
      break;
    case 1:
      DBMSG(DBL_ALL_BLE_EVENTS, "current production ran a rule")  
      break;
    default:
      DBMSG(DBL_ALL_BLE_EVENTS, "current production returned unexpected result")  
  };
}

void set_current_protocol(protocol_ptr_t protocol) {
  current_protocol = protocol;
}

protocol_ptr_t get_current_protocol() {
  return current_protocol;
}

void clear_current_protocol() {
    clear_expectations();           
    clear_exclusive_expectations(); 
    until_clear();                  
    until_event_clear();            
    current_protocol = NULL;
}

char protocol_name[MAX_PROTOCOL_STRING_SIZE];
void set_protocol_name(char *proto_name) { strncpy(protocol_name, proto_name, MAX_PROTOCOL_STRING_SIZE); }
char * get_protocol_name() { return protocol_name; }

void wait_for_protocol_finish() {
  while (current_protocol) delay(500);
}

bool protocol_running() {
  return (current_protocol != NULL);
}
