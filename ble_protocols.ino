/*!
 * @file ble_protocols.ino
 * @brief Example of scannng for BLE devices and enumerating their services and characteristics using the protocol and production framework.
 * @details
 * There are two main protocols involved 
 * 1. directed_scan_protocol (or optionally observation_protocol for passive scanning) to find all devices in the area
 * 2. gatt_walk_protocol - adds all services and characteristics to the device db for one device
 * - one to discover devices (passively or actively) and one to add all services and characteristics to the device db.   
 * To coordinate two different protocols, a step function is used. The first step is discover devices and the second step invokes the gatt_walk_protocol once
 * for each device that was previously found.
 * 
 * This sketch makes heavy use of protocols, productions, procedures, etc. from the rest of this project.
 * 
 * See sample_output.txt for an example of running this sketch.
 * 
 * Please note that Doxygen will not parse the protocol/production/step language so you will see little in Doxygen documentation for this file. 
 * If you are reading this in the html documentation, then click on this linke to read the source file for this ==> ble_productions_source.txt
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <SPI.h>
#include <stdbool.h>
#include <stdint.h>
#include "dbprint.h"
#include "production.h"
#include "protocol.h"
#include "hci.h"
#include "procedures.h"
#include "get_data.h"
#include "addrs.h"
#include "db.h"

// Show debug messages for first 5 minutes 
#define FIVE_MINUTES 60000 * 5

// Number of milliseconds in a second
#define SECONDS 1000


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// global event handlers (when protocol specific expectations are not met)
//
////////////////////////////////////////////////////////////////////////////////////////////////

void set_global_expectations() {
  expect_globally_condition(check_initialization_or_reset, NO_ACTION, NO_ARGS);
  expect_globally_condition(check_event, NO_ACTION, NO_ARGS);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// use advertising data to build a list of connectable public addresses
//
////////////////////////////////////////////////////////////////////////////////////////////////

bool process_advertising_info(hci_event_pckt *event_pckt, DUMMY_ARG) {
  ble_advertising_info_t * info = get_advertising_info(event_pckt);
  if ( (info->evt_type == ADV_IND) || (info->evt_type == ADV_DIRECT_IND) || (info->evt_type == ADV_SCAN_IND)|| (info->evt_type == SCAN_RSP)) {
    if (info->bdaddr_type == PUBLIC_ADDR) add_addr(info->bdaddr, true, true);
    else add_addr(info->bdaddr, true, false);
  }
  else {
    if (info->bdaddr_type == PUBLIC_ADDR) add_addr(info->bdaddr, false, true);
    else add_addr(info->bdaddr, false, false);
  }
  DBADDR(DBL_IMPORTANT_EVENTS, info->bdaddr, "Device Address")
  DBPR(DBL_DECODED_EVENTS, info->rssi_value, "%d", "RSSI")
  DBPRNS(DBL_DECODED_EVENTS, info->data, info->data_length, "Advertising Data in char format");
  DBPRN(DBL_DECODED_EVENTS, info->data, info->data_length,  "Advertising Data in hex format ");
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// observation protocol
//
////////////////////////////////////////////////////////////////////////////////////////////////

PROTOCOL(observation_protocol)
  BEGIN_PROTOCOL(observation_protocol)
    init_addr_list();
    PERFORM(start_HCI, WITH(NO_ARGS));
      expect(reset_reason, SPECIFICALLY(RESET_NORMAL), AND_DO(set_MAC_addr_action), WITH(NO_ARGS));
    RUN_PRODUCTION
    if (met_expectations()) {
      set_timeout(20 * SECONDS);
      PERFORM(start_observation, WITH(NO_ARGS));
        expect(event_check, SPECIFICALLY(EVT_LE_META_EVENT), AND_DO(process_advertising_info), WITH(NO_ARGS));
        until(timeout);
        PROTOCOL_IS_WORKING
    }
    else {
      PRINTF("observation failed to start\n");
      ABORT_PROTOCOL
    }
    RUN_PRODUCTION
    BlueNRG_RST();   // stop processing events
    PRINTF("observation ended\n");
    if (protocol_success) print_addrs();
  END_PROTOCOL 

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// directed scan protocol
//
////////////////////////////////////////////////////////////////////////////////////////////////

PROTOCOL(directed_scan_protocol)
  BEGIN_PROTOCOL(directed_scan_protocol)
    init_addr_list();
    PERFORM(start_HCI, WITH(NO_ARGS));
      expect(reset_reason, SPECIFICALLY(RESET_NORMAL), AND_DO(set_MAC_addr_action), WITH(NO_ARGS));
    RUN_PRODUCTION
    if (met_expectations()) {
      PERFORM(start_directed_scan, WITH(NO_ARGS));
        expect(event_check, SPECIFICALLY(EVT_LE_META_EVENT), AND_DO(process_advertising_info), WITH(NO_ARGS));
        until_event(procedure_complete, SPECIFICALLY(GAP_GENERAL_DISCOVERY_PROC)); 
        PROTOCOL_IS_WORKING
    }
    else {
      PRINTF("general discovery failed to start\n");
      ABORT_PROTOCOL
    }
    RUN_PRODUCTION
    PRINTF("general discovery ended\n");
    if (IS_PROTOCOL_WORKING) print_addrs();
  END_PROTOCOL 

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Gatt walk protocol - enumerate all services and characteristics
//
// Overview:
//  1) connect
//  2) find all primary services
//  3) optionally find all included services (define TRY_INCLUDED_SERVICES to make this happen)
//  4) find all characteristics for each service
//
////////////////////////////////////////////////////////////////////////////////////////////////

tBDAddr addr2walk;
uint16_t connection_handle;

PROTOCOL(gatt_walk_protocol)
  evt_disconn_complete *disconnection_complete_pckt;
  static int device_index, service_index;
  bool items_todo;
  static attribute_info_t * discovery_args;
  static attribute_context_t context;
  BEGIN_PROTOCOL(gatt_walk_protocol)
    PERFORM(start_connection, WITH(&addr2walk))
      expect_ex(le_meta_event_check, SPECIFICALLY(EVT_LE_CONN_COMPLETE), AND_DO(get_connection_handle), WITH(&connection_handle));
      expect_ex(procedure_complete, SPECIFICALLY(GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC), AND_DO(display_event), WITH(NO_ARGS));
      until_event(le_meta_event_check, SPECIFICALLY(EVT_LE_CONN_COMPLETE));
    RUN_PRODUCTION
    if (met_expectations()) {
      PROTOCOL_IS_WORKING
      device_index = add_device_to_device_db(&addr2walk);
      set_context(&context, db_primary_service, PARENT(device_index), connection_handle);
      mark_processed_in_device_db(device_index);
      PRINTF("starting discovery primary services\n")
      PERFORM(discover_primary_services, WITH(&connection_handle));
        expect_ex(ecode, EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP, AND_DO(add_device_db_entry_from_event), WITH(&context));
        expect_ex(ecode, EVT_BLUE_L2CAP_CONN_UPD_REQ, AND_DO(handle_connection_update), WITH(NO_ARGS));
        until_event(ecode, EVT_BLUE_GATT_PROCEDURE_COMPLETE); 
    }
    else {
      PRINTF("gatt_walk_protocol failed to start connection\n");
      ABORT_PROTOCOL
    }
    RUN_PRODUCTION
    #ifdef TRY_INCLUDED_SERVICES
    if (IS_PROTOCOL_WORKING) {
      PRINTF("starting discovery of included services (%04X)\n", connection_handle)
      service_index = RECALL_PRIMARY_SERVICE(device_index)
      if (service_index < 0) {
        PRINTF("no primary service found to enumerate\n")        
        ABORT_PROTOCOL
      }
      discovery_args = get_attribute_info_from_device_db(service_index);
      set_context(&context, db_included_service, PARENT(service_index), connection_handle);
      mark_processed_in_device_db(service_index);
      PERFORM(discover_included_services, WITH(discovery_args));
        expect_ex(ecode, EVT_BLUE_ATT_READ_BY_TYPE_RESP, AND_DO(add_device_db_entry_from_event), WITH(&context));
        expect_ex(ecode, EVT_BLUE_L2CAP_CONN_UPD_REQ, AND_DO(handle_connection_update), WITH(NO_ARGS));
        until_event(ecode, EVT_BLUE_GATT_PROCEDURE_COMPLETE); 
    }
    else {
      PRINTF("could not start discovery of included services; expectations not met\n");
      ABORT_PROTOCOL
    }
    items_todo = PRIMARY_SERVICES_TODO(device_index)
    if (!items_todo) {
      PRINTF("resetting primary services\n")
      RESET_ALL_PRIMARY_SERVICES(device_index) // now that we have included services for all primary services, prepare to start over, getting characteristics
      service_index = RECALL_PRIMARY_SERVICE(device_index)
      if (service_index < 0) {
        PRINTF("reset appears to have failed \n")
        ABORT_PROTOCOL
      }
    }
    else PRINTF("items todo: %d\n", num_unprocessed_of_type_for_device_in_device_db(db_primary_service, device_index));
    RUN_PRODUCTION_AND_REPEAT_IF(items_todo)
    #endif
    if (IS_PROTOCOL_WORKING) {
      PRINTF("starting discovery of characteristics\n")
      service_index = RECALL_PRIMARY_SERVICE(device_index)
      if (service_index < 0) service_index = RECALL_INCLUDED_SERVICE(device_index)
      if (service_index < 0) {
        PRINTF("no service found to enumerate \n")
        ABORT_PROTOCOL
      }
      discovery_args = get_attribute_info_from_device_db(service_index);
      set_context(&context, db_characteristic, PARENT(service_index), connection_handle);
      mark_processed_in_device_db(service_index);
      PERFORM(discover_characteristcs, WITH(discovery_args));
        expect_ex(ecode, EVT_BLUE_ATT_READ_BY_TYPE_RESP, AND_DO(add_device_db_entry_from_event), WITH(&context));
        expect_ex(ecode, EVT_BLUE_L2CAP_CONN_UPD_REQ, AND_DO(handle_connection_update), WITH(NO_ARGS));
        until_event(ecode, EVT_BLUE_GATT_PROCEDURE_COMPLETE); 
    }
    else {
      PRINTF("could not start discovery of included services; expectations not met\n");
      ABORT_PROTOCOL
    }
    items_todo = PRIMARY_SERVICES_TODO(device_index)
    if (!items_todo) items_todo = INCLUDED_SERVICES_TODO(device_index)
    RUN_PRODUCTION_AND_REPEAT_IF(items_todo)
    if (IS_PROTOCOL_WORKING) {
      PRINTF("terminating connection\n")
      PERFORM(terminate_connection, WITH(&connection_handle));
        expect_ex(event_check, SPECIFICALLY(EVT_DISCONN_COMPLETE), AND_DO(get_disconnection_complete), WITH(&disconnection_complete_pckt));
        until_event(event_check, SPECIFICALLY(EVT_DISCONN_COMPLETE)); 
    }
    else {
      PRINTF("gatt_walk_protocol failed\n");
      ABORT_PROTOCOL
    }
    RUN_PRODUCTION
    if (IS_PROTOCOL_WORKING && disconnection_complete_pckt->status == BLE_STATUS_SUCCESS) {
      DBMSG(DBL_HAL_EVENTS, "Disconnection successful.")
    }
    else {
      DBPR(DBL_HAL_EVENTS, disconnection_complete_pckt->status, "%02X", "Disconnection unsuccessful.")
    }
    PRINTF("gatt_walk_protocol ended\n");
  END_PROTOCOL 

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// run desired protocols
//
////////////////////////////////////////////////////////////////////////////////////////////////

STEP_FUNCTION(main_steps)
  int connectable, public_addr;
  bool is_next_addr;
  SKIP_STEPS_IF(protocol_running())
  FIRST_STEP 
    directed_scan_protocol();
  NEXT_STEP
    DB_set_lvl(9);
    addr_enumeration_start();
  NEXT_STEP
    is_next_addr = addr_enumeration_next(&addr2walk, &connectable, &public_addr);
    if (is_next_addr && (connectable==1) && (public_addr==1) ) gatt_walk_protocol();
    REPEAT_STEP_WHILE(is_next_addr)
  NEXT_STEP
    BlueNRG_RST();   // stop processing events
    PRINTF("Devices, services, and characteristics found\n")
    dump_device_db();
    print_device_db();
    PRINTF("main_steps ended\n");
END_STEP_FUNCTION 

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// standard setup
//
////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  SerialUSB.begin(115200);
  while (!SerialUSB);  // block until a serial monitor is opened with TinyScreen+
  DB_print_for(FIVE_MINUTES);   
  set_global_expectations();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// standard loop and event call back; should not need to change these
//
////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  main_steps();
  HCI_Process();
}

void HCI_Event_CB(void *pckt) {
  run_current_protocol(pckt);
}
 
