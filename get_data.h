/*!
 * @file get_data.h
 * @brief provides procedures to extra useful data from events 
 * @details
 * These are used by procedures.h/.cpp so that actions can use data from events to do something useful.
 * Unless you are implementing your own custom actions, you can view this as just being used internally.
 * 
 * The data structures used somewhat duplicate the corresponding BLNRG data structure except
 * that there are no variable length arrays, allowing statically declared instances to be used. This is really
 * why the BLNRG structures are duplicated. But since we are duplicating them to avoid variable length, some
 * liberty has been taken to simplify a few things (e.g., separating RSSI from the data).
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ADVERTISING_H
#define ADVERTISING_H

#include "HCI.h"
#include "dbprint.h"

#define MAX_VALUE_LEN 10

typedef struct uuid_type_s {
  bool    is_16_bit;
  uint8_t bytes[16];
} uuid_t;

void copy_uuid(uuid_t * from, uuid_t * to);

// the intent of the following is to contain the information from the attribute list returned when discovering services and characteristics + connection_handle for later discovery
typedef struct attribute_info_s {
  uint16_t connection_handle;   
  uint16_t starting_handle;
  uint16_t ending_handle;
  uuid_t   uuid;
} attribute_info_t;

void get_attribute_info(uint8_t * attr_list, int attr_len, attribute_info_t * attr);
void copy_attribute_info(attribute_info_t * from, attribute_info_t * to);

typedef struct handle_value_pair_s {
  uint16_t connection_handle;
  uint16_t handle;
  uint8_t  len;
  uint8_t  value[MAX_VALUE_LEN];
} handle_value_pair_t;

void get_handle_value_pair(uint8_t * handle_value_pair_list, int handle_value_pair_len, handle_value_pair_t * handle_value_pair);
void copy_handle_vlaue_pair(handle_value_pair_t * from, handle_value_pair_t * to);

  /* le_advertising_info fields:
   *   uint8_t evt_type: event type (advertising packets types);
   *   uint8_t bdaddr_type: type of the peer address (PUBLIC_ADDR,RANDOM_ADDR);
   *   tBDAddr bdaddr: address of the peer device found during scanning;
   *   uint8_t data_length: length of advertising or scan response data;
   *   uint8_t data_RSSI[]: length advertising or scan response data + RSSI (RSSI is last octect (signed integer)).
  */
//Note that BLENRG types are declared with VARIABLE_SIZE (i.e., 0) sized arrays so we need a different type 
typedef struct ble_advertising_info_s {
  uint8_t evt_type;
  uint8_t bdaddr_type;
  tBDAddr bdaddr;
  uint8_t data_length;
  uint8_t data[300]; 
  int8_t  rssi_value;
} ble_advertising_info_t;

ble_advertising_info_t * get_advertising_info(hci_event_pckt *event_pckt);

bool get_connection_handle(hci_event_pckt *event_pckt, void * connection_handle);

bool get_disconnection_complete(hci_event_pckt *event_pckt, void * of_type_ptr_to_ptr_to_evt_disconn_complete);

void print_attr_list(uint8_t * attr_list, uint8_t total_len, uint8_t attr_len);

void print_uuid(uuid_t * uuid);


#endif
