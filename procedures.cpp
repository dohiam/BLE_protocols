/*!
 * @file procedures.h
 * @brief provides procedures that can be used as things to perform or actions to do when an expectation is met
 * @details
 * In addition to wrapping various BlueRNG procedure functions with a signature that can be used by the framework, these 
 * implementations also include error checking
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * These are used to retrieve the results of an event, pretty much duplicating the BLNRG data structures except
 * that there are no variable length arrays, allowing statically declared instances to be used. This is really
 * why the BLNRG structures are duplicated. But since we are duplicating them to avoid variable length, some
 * liberty has been taken to simplify a few things (e.g., separating RSSI from the data).
 * 
 * David Hamilton, 2021
 * 
 */


#include "dbprint.h"
#include "production.h"

#include "procedures.h"
#include "HCI.h"

// According to [c] the following should be "0x0004 to 0x4000. This corresponds to a time range from 2.5 msec to 10240 msec. For a number N, Time = N * 0.625 msec."
#define TIME_BETWEEN_SCANS 16000

// According to [c] the following should be the same format as above (0x0004 to 0x4000 and is time / .0625)
#define TIME_TO_SCAN 6400

// According to [c], 0x00: Do not filter the duplicates, 0x01: Filter duplicates
#define DO_NOT_FILTER_DUPLICATES 0x00
#define FILTER_DUPLICATES 0x01

// These definitions come from information in [3] in the comments for the gap init function.
#define PRIVACY_DISABLED 0
#define PRIVACY_ENABLED 1

bool set_role_to_observer() {
  tBleStatus ret;
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  const char * device_name = get_device_name();
  ret = aci_gatt_init();
  if (ret) {
    DBMSG(DBL_ERRORS, "GATT_Init failed.")
    DBPR(DBL_ERRORS, ret, "%d", "return code")
    hci_print_ret(ret);
    return false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "GATT_Init succeeded")
  }
  ret = aci_gap_init_IDB05A1(GAP_OBSERVER_ROLE_IDB05A1, PRIVACY_DISABLED, strlen(device_name), &service_handle, &dev_name_char_handle, &appearance_char_handle);
  if (ret) {
    DBMSG(DBL_ERRORS, "GAP_Init failed.")
    DBPR(DBL_ERRORS, ret, "%d", "return code")
    hci_print_ret(ret);
    return false;
  } else {
    DBMSG(DBL_ALL_BLE_EVENTS, "GAP initialized.")
  }
  ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(device_name), (uint8_t *)device_name);
  if (ret) {
    DBMSG(DBL_ERRORS, "aci_gatt_update_char_value failed.")
    DBPR(DBL_ERRORS, ret, "%d", "return code");
    hci_print_ret(ret);
    return false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "BLE Stack Initialized.");
    return true;
  }
}

// Kick off the scan
bool start_observer_scan() {
  tBleStatus ret;
  ret = aci_gap_start_observation_procedure(TIME_BETWEEN_SCANS, TIME_TO_SCAN, PASSIVE_SCAN, PUBLIC_ADDR, DO_NOT_FILTER_DUPLICATES);
  if (ret != BLE_STATUS_SUCCESS) {
    DBMSG(DBL_ERRORS, "Failure to start observer scan!")
    DBPR(DBL_ERRORS, ret, "%d", "return code");
    return false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "started observer scan")
    return true;
  }
}

bool start_observation(DUMMY_ARG) {
  if (!set_role_to_observer()) return false;
  if (!start_observer_scan()) return false;
  return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

// set role to central
bool set_role_central() {
  tBleStatus ret;
  bool success;
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  const char * device_name = get_device_name();
  ret = aci_gatt_init();
  if (ret) {
    DBMSG(DBL_ERRORS, "*** GATT_Init failed.")
    hci_print_ret(ret);
    success = false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "GATT_Init succeeded.")
    success = true;
  }
  ret = aci_gap_init_IDB05A1(GAP_CENTRAL_ROLE_IDB05A1, PRIVACY_DISABLED, strlen(device_name), &service_handle, &dev_name_char_handle, &appearance_char_handle);
  if (ret) {
    DBMSG(DBL_ERRORS, "*** GAP_Init failed.")
    hci_print_ret(ret);
    success = false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "GAP initialized.");
    success = true;
  }
  ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(device_name), (uint8_t *)device_name);
  if (ret) {
    DBMSG(DBL_ERRORS, "*** aci_gatt_update_char_value failed.")
    hci_print_ret(ret);
    success = false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "BLE Stack Initialized.");
    success = true;
  }
  return success;
}

bool start_general_discovery() {
  tBleStatus ret;
  bool success;
  DBMSG(DBL_HAL_EVENTS, "Starting directed scan.")
  ret = aci_gap_start_general_discovery_proc(TIME_BETWEEN_SCANS, TIME_TO_SCAN, PUBLIC_ADDR, FILTER_DUPLICATES);
  if (ret != BLE_STATUS_SUCCESS) {
    DBMSG(DBL_ERRORS, "*** Failure to start general discovery!")
    hci_print_ret(ret);
    success = false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "started general discovery")
    success = true;
  }
  return success;
}

bool start_directed_scan(DUMMY_ARG) {
  if (!set_role_central()) return false;
  if (!start_general_discovery()) return false;
  return true;
  set_role_central();
  start_general_discovery();    
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned long connect_time;

// Start a connection. This should result in a EVT_LE_CONN_COMPLETE event with a corresponding evt_le_connection_complete data structure
bool start_connection(arg_t addr_arg) {
  tBDAddr * addr = (tBDAddr *) addr_arg;
  tBleStatus ret;
  DBADDR(DBL_HAL_EVENTS, (*addr), "creating connection for")
  ret = aci_gap_create_connection(0x4000, 0x4000, PUBLIC_ADDR, *addr, PUBLIC_ADDR, 40, 40, 0, 60, 2000 , 2000);
  if (ret) {
    hci_print_ret(ret);
    DBMSG(DBL_ERRORS, "*** Create connection failed failed.")
    DBADDR(DBL_ERRORS, *addr, "failed to create connection to")
    return false;
  } else {
    DBMSG(DBL_HAL_EVENTS, "Create Connection succeeded.")
    DBADDR(DBL_HAL_EVENTS, (*addr), "connection for")
    connect_time = millis();
    return true;
  }
}

void process_connection_update_request(evt_l2cap_conn_upd_req * connection_update_request_pckt);

bool handle_connection_update(hci_event_pckt *event_pckt, DUMMY_ARG) {
  evt_blue_aci *evt_blue;
  evt_l2cap_conn_upd_req * connection_update_request_pckt;
  if (event_pckt->evt == EVT_VENDOR) {
    evt_blue = (evt_blue_aci *) (event_pckt->data);
    if (evt_blue->ecode == EVT_BLUE_L2CAP_CONN_UPD_REQ) {
      DBMSG(DBL_HCI_EVENTS, "Responding to connection update request")
      connection_update_request_pckt = (evt_l2cap_conn_upd_req *) evt_blue->data;
      process_connection_update_request(connection_update_request_pckt);
      return true;   
    }
    else DBMSG(DBL_ERRORS, "handle_connection_update expected EVT_BLUE_L2CAP_CONN_UPD_REQ")
  }
  else DBMSG(DBL_ERRORS, "handle_connection_update expected EVT_VENDOR")  
  return false;
}

/* just acknowledge and accept the connection parameters, as given */
void process_connection_update_request(evt_l2cap_conn_upd_req * connection_update_request_pckt) {
  tBleStatus ret;
  /* evt_l2cap_conn_upd_req contains the following (bluenrg_l2cap_aci.h)
   * uint16_t conn_handle; Handle of the connection for which the connection update request has been received.
   * uint8_t  event_data_length; Length of following data.
   * uint8_t  identifier; This is the identifier which associates the request to the response. 
   *                      The same identifier has to be returned by the upper layer in the command 
   *                      aci_l2cap_connection_parameter_update_response().
   * uint16_t l2cap_length;  Length of the L2CAP connection update request.
   * uint16_t interval_min;  Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20. 
   * uint16_t interval_max;  Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20. 
   * uint16_t slave_latency; Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20. 
   * uint16_t timeout_mult;  Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20.
   */
   ret = aci_l2cap_connection_parameter_update_response_IDB05A1(connection_update_request_pckt->conn_handle, 
                                                                connection_update_request_pckt->interval_min, 
                                                                connection_update_request_pckt->interval_max, 
                                                                connection_update_request_pckt->slave_latency,
                                                                connection_update_request_pckt->timeout_mult, 
                                                                0, 0xFFFF, /* min and max connection time length */
                                                                connection_update_request_pckt->identifier, 
                                                                1); /* the connection parameters are acceptable */
   if (ret) {
      DBMSG(DBL_ERRORS, "*** Create connection update response failed.")
      DBPR(DBL_ERRORS, ret, "%d\n", "Return code")
    } else {
      DBMSG(DBL_HAL_EVENTS, "Create Connection update response succeeded.")
    }
}


// should result in a EVT_DISCONN_COMPLETE
bool terminate_connection(arg_t ptr_to_connection_handle) {
  tBleStatus ret;
  uint16_t connection_handle = *((uint16_t *) ptr_to_connection_handle);
  ret = aci_gap_terminate(connection_handle, HCI_CONNECTION_TERMINATED);
  if (ret) {
    DBMSG(DBL_ERRORS, "*** Terminate connection failed.")
    DBPR(DBL_ERRORS, ret, "%d\n", "Return code")
    hci_print_ret(ret);
  } 
  else {
    DBMSG(DBL_HAL_EVENTS, "Terminate Connection succeeded.")
  }
}

bool terminate_gap_procedure(arg_t ptr_to_procedure_code) {
  tBleStatus ret;
  uint8_t procedure_code = *((uint8_t *) ptr_to_procedure_code);
  ret = aci_gap_terminate_gap_procedure(procedure_code);
  if (ret) {
    DBMSG(DBL_ERRORS, "*** Terminate gap procedure failed.")
    DBPR(DBL_ERRORS, ret, "%d\n", "Return code")
    hci_print_ret(ret);
  } 
  else {
    DBMSG(DBL_HAL_EVENTS, "Terminate Connection succeeded.")
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

bool discover_primary_services(arg_t connection_handle_arg) {
  tBleStatus ret;
  uint16_t * connection_handle = (uint16_t *) connection_handle_arg;
  ret = aci_gatt_disc_all_prim_services(*connection_handle);
  switch (ret) {
    case BLE_STATUS_TIMEOUT: 
      DBMSG(DBL_HAL_EVENTS, "discover all primary services had a timeout, continuing.")
      return true;
    case BLE_STATUS_SUCCESS:
      DBMSG(DBL_HAL_EVENTS, "discover all primary services succeeded.")
      return true;
    default:
      DBMSG(DBL_ERRORS, "*** discover all primary services failed.")
      hci_print_ret(ret);
      return false;
  };
}

bool discover_included_services(arg_t attribute_info) {
  tBleStatus ret;
  attribute_info_t * args = (attribute_info_t *) attribute_info;
  DBPR(DBL_ERRORS, args->connection_handle, "%04X", "connection handle for finding included services")
  DBPR(DBL_ERRORS, args->starting_handle, "%04X", "starting handle for finding included services")
  DBPR(DBL_ERRORS, args->ending_handle, "%04X", "ending handle for finding included services")
  ret = aci_gatt_find_included_services(args->connection_handle, args->starting_handle, args->ending_handle);
  switch (ret) {
    case BLE_STATUS_TIMEOUT: 
      DBMSG(DBL_HAL_EVENTS, "find included services had a timeout, continuing.")
      return true;
    case BLE_STATUS_SUCCESS:
      DBMSG(DBL_HAL_EVENTS, "find included services succeeded.")
      return true;
    default:
      DBMSG(DBL_ERRORS, "*** find included services failed.")
      hci_print_ret(ret);
      return false;
  };
}

bool discover_characteristcs(arg_t attribute_info) {
  tBleStatus ret;
  attribute_info_t * args = (attribute_info_t *) attribute_info;
  ret = aci_gatt_disc_all_charac_of_serv(args->connection_handle, args->starting_handle, args->ending_handle);
  switch (ret) {
    case BLE_STATUS_TIMEOUT: 
      DBMSG(DBL_HAL_EVENTS, "discover all characteristics had a timeout, continuing.")
      return true;
    case BLE_STATUS_SUCCESS:
      DBMSG(DBL_HAL_EVENTS, "discover all characteristics succeeded.")
      return true;
    default:
      DBMSG(DBL_ERRORS, "*** discover all characteristics failed.")
      hci_print_ret(ret);
      return false;
  };
}
