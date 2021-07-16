/*!
 * @file get_data.h
 * @brief implementations of procedures to extra useful data from events 
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


#include "get_data.h"
#include "addrs.h"
#include "dbprint.h"

void copy_uuid(uuid_t * from, uuid_t * to) {
  int i, size;
  to->is_16_bit = from->is_16_bit;
  if (from->is_16_bit) size = 2;
  else size = 16;
  for (i=0; i<size; i++) to->bytes[i] = from->bytes[i];
}


void get_attribute_info(uint8_t * attr_list, int attr_len, attribute_info_t * attr) {
  if ( (attr_len < 6) || (6 < attr_len) && (attr_len != 20) ) {
    PRINTF("get_attribute_info with wrong attr_len: %d \n", attr_len);
    return; 
  }
  attr->starting_handle = attr_list[1];
  attr->starting_handle = (attr->starting_handle << 8) + attr_list[0];
  attr->ending_handle = attr_list[3];
  attr->ending_handle = (attr->ending_handle << 8) + attr_list[2];
  if (attr_len == 6) attr->uuid.is_16_bit = true;
  else attr->uuid.is_16_bit = false;
  for (int i = 4; i < attr_len; i++) attr->uuid.bytes[i-4] = attr_list[i];
}

void copy_attribute_info(attribute_info_t * from, attribute_info_t * to) {
  to->starting_handle = from->starting_handle;
  to->ending_handle = from->ending_handle;
  copy_uuid(&(from->uuid), &(to->uuid));
}

void get_handle_value_pair(uint8_t * handle_value_pair_list, int handle_value_pair_len, handle_value_pair_t * handle_value_pair) {
  handle_value_pair->handle = handle_value_pair_list[1];
  handle_value_pair->handle = (handle_value_pair->handle << 8) + handle_value_pair_list[0];
  handle_value_pair->len = handle_value_pair_len - 2;
  for (int i = 0; i < handle_value_pair->len; i++) handle_value_pair->value[i] = handle_value_pair_list[i+2];
}

void copy_handle_vlaue_pair(handle_value_pair_t * from, handle_value_pair_t * to) {
  to->handle = from->handle;
  to->len = from->len;
  for (int i=0; (i < from->len) && (i < MAX_VALUE_LEN); i++) to->value[i] = from->value[i];
}


ble_advertising_info_t advertising_info;

ble_advertising_info_t * get_advertising_info(hci_event_pckt *event_pckt) {
  int index;
  evt_le_meta_event *report_event_pckt;
  report_event_pckt = (evt_le_meta_event *) event_pckt->data;
  le_advertising_info *report_pckt = (le_advertising_info *) (report_event_pckt->data+1); 
  advertising_info.evt_type = report_pckt->evt_type;
  advertising_info.bdaddr_type = report_pckt->bdaddr_type;
  copy_addr(report_pckt->bdaddr, &(advertising_info.bdaddr));
  advertising_info.data_length = report_pckt->data_length;
  for (index = 0; index < advertising_info.data_length; index++) advertising_info.data[index] = report_pckt->data_RSSI[index];
  advertising_info.rssi_value = report_pckt->data_RSSI[advertising_info.data_length];
  return &advertising_info;
}

bool get_connection_handle(hci_event_pckt *event_pckt, void * connection_handle_arg) {
  uint16_t * connection_handle = (uint16_t *) connection_handle_arg;
  evt_le_meta_event *report_event_pckt;
  evt_le_connection_complete *connecton_complete_pckt;
  if (event_pckt->evt == EVT_LE_META_EVENT) {
    report_event_pckt = (evt_le_meta_event *) event_pckt->data;
    if (report_event_pckt->subevent == EVT_LE_CONN_COMPLETE) {
      connecton_complete_pckt = (evt_le_connection_complete *) report_event_pckt->data;
      if (connecton_complete_pckt->status != 0) {
        DBMSG(DBL_ERRORS, "*** Failure to create connection!")
        DBPR(DBL_ERRORS, connecton_complete_pckt->status, "%d", "status code");
      } 
      else {
        DBMSG(DBL_HAL_EVENTS, "connection created successfully")
        *connection_handle = connecton_complete_pckt->handle;
        DBPR(DBL_HAL_EVENTS, *connection_handle, "%d", "returned by get_connection_handle");
        return true;
      }
    }
  }
  DBMSG(DBL_ERRORS, "called get_connection_handle on wrong event")
  *connection_handle = 0;
  return false;
}
bool get_disconnection_complete(hci_event_pckt *event_pckt, void * of_type_ptr_to_ptr_to_evt_disconn_complete) {
  if (event_pckt->evt == EVT_DISCONN_COMPLETE) {
    *((evt_disconn_complete **)of_type_ptr_to_ptr_to_evt_disconn_complete) = (evt_disconn_complete *) event_pckt->data;
    return true;
  }
  else {
    DBMSG(DBL_ERRORS, "*** Called get_disconnection_complete on wrong event type")
    *((evt_disconn_complete **)of_type_ptr_to_ptr_to_evt_disconn_complete) = NULL;
    return true;
  }
}

void print_attr_list(uint8_t * attr_list, uint8_t total_len, uint8_t attr_len) {
  uint8_t i;
  uint8_t * attr_start;
  DBMSG(DBL_DECODED_EVENTS, "attribute list:")
  for (i=0; i < total_len; i += attr_len) {
    attr_start = attr_list + i;
    DBPRN(DBL_DECODED_EVENTS, attr_start, attr_len, "attribute")
  }
}

void print_uuid(uuid_t * uuid) {
  int i, size;
  PRINTF("uuid ");
  if (uuid->is_16_bit) size = 2;
  else size = 16;
  for (i=size-1; i>0; i--) PRINTF("%02x", uuid->bytes[i]);
  PRINTF("%02x ", uuid->bytes[0]);
}
