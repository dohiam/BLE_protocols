/*!
 * @file /HCI.h
 * @brief A collection of common operations for BlueNRG HCI.
 * @details
 * A key resource provided is global handling of all BlueRNG events. This is done by an event condition function instead of adding each event
 * to the global rules individually. These functions go through cascading switch statements to try to cover all possible events according to
 * what I found in the STBLE header files, printint some approprate debug information. So debug output needs to be enabled; otherwise these
 * don't really do anything.
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "HCI.h"
#include "dbprint.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// standard BNRG startup
//
////////////////////////////////////////////////////////////////////////////////////////////////

bool start_HCI(DUMMY_ARG) {
  HCI_Init();                   // Initialize internal data structures used to process HCI commands       
  BNRG_SPI_Init();              // Initialize the SPI interface to the module
  BlueNRG_RST();                // Reset the BLE processor to start it taking commands
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// set device name and mac address
//
////////////////////////////////////////////////////////////////////////////////////////////////

// device name
const char hci_device_name[] = OUR_DEVICE_NAME;

const char * get_device_name() { return hci_device_name; }

// MAC address
void set_public_MAC_addr() {
  tBleStatus ret;
  uint8_t bdaddr[] = OUR_MAC_ADDR;
  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bdaddr);
  if (ret) {
    DBMSG(DBL_ERRORS, "Setting BD_ADDR failed.")
    DBPR(DBL_ERRORS, ret, "%d", "return code")
  } else {
    DBMSG(DBL_HAL_EVENTS, "public address set")
  }
}

// do both
bool set_MAC_addr_action(hci_event_pckt *event_pckt, DUMMY_ARG) {
  set_public_MAC_addr();        
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Handle events related to initializaton or reset
//
////////////////////////////////////////////////////////////////////////////////////////////////

bool check_initialization_or_reset(hci_event_pckt *event_pckt) { display_initialization_or_reset(event_pckt, NO_ARGS); }

bool display_initialization_or_reset(hci_event_pckt *event_pckt, DUMMY_ARG) {
  bool handled = false;
  evt_blue_aci *evt_blue;
  evt_hal_initialized * reset_pckt; 
  uint8_t reason_code;
  if (event_pckt->evt == EVT_VENDOR) {
    evt_blue = (evt_blue_aci *) (event_pckt->data);
    if (evt_blue->ecode == EVT_BLUE_HAL_INITIALIZED) {
      DBMSG(DBL_HAL_EVENTS, "HAL initialized or reset")
      handled = true;
      reset_pckt = (evt_hal_initialized *) (event_pckt->data);
      reason_code = reset_pckt->reason_code;
      switch (reason_code) {
        /* from bluerng_hal_aci.h */
        case RESET_NORMAL:            DBMSG(DBL_HAL_EVENTS, "Normal startup.") break;
        case RESET_UPDATER_ACI:       DBMSG(DBL_HAL_EVENTS, "Updater mode entered with ACI command") break;
        case RESET_UPDATER_BAD_FLAG:  DBMSG(DBL_ERRORS,     "Updater mode entered due to a bad BLUE flag") break;
        case RESET_UPDATER_PIN:       DBMSG(DBL_HAL_EVENTS, "Updater mode entered with IRQ pin") break;
        case RESET_WATCHDOG:          DBMSG(DBL_ERRORS,     "Reset caused by watchdog") break;
        case RESET_LOCKUP:            DBMSG(DBL_ERRORS,     "Reset due to lockup") break;
        case RESET_BROWNOUT:          DBMSG(DBL_ERRORS,     "Brownout reset") break;
        case RESET_CRASH:             DBMSG(DBL_ERRORS,     "Reset caused by a crash (NMI or Hard Fault)") break;
        case RESET_ECC_ERR:           DBMSG(DBL_ERRORS,     "Reset caused by an ECC error") break;
        default:                      DBMSG(DBL_ERRORS,     "Reset caused by unknown reason") handled = false;
      }
    }
  }
  return handled;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// handle BlueRNG-MS vendor codes
//
////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// generic event handler to dispaly events
//
////////////////////////////////////////////////////////////////////////////////////////////////

bool display_meta_event(evt_le_meta_event *report_event_pckt);
bool display_ecode(evt_blue_aci *evt_blue);

bool check_event(hci_event_pckt *event_pckt) { display_event(event_pckt, NO_ARGS); }

bool display_event(hci_event_pckt *event_pckt, DUMMY_ARG) {
  bool handled = true;
  evt_le_meta_event *report_event_pckt;
  evt_blue_aci *evt_blue;
  DBBUFF(DBL_RAW_EVENT_DATA, event_pckt)
  DBPR(DBL_RAW_EVENT_DATA, event_pckt->evt, "%d", "event code")
  switch(event_pckt->evt) {  
    CASE_PRINT_ENUM(EVT_CONN_COMPLETE)                          /* 0x03 */
    CASE_PRINT_ENUM(EVT_DISCONN_COMPLETE)                       /* 0x05 */
    CASE_PRINT_ENUM(EVT_ENCRYPT_CHANGE)                         /* 0x08 */
    CASE_PRINT_ENUM(EVT_READ_REMOTE_VERSION_COMPLETE)           /* 0x0C */
    CASE_PRINT_ENUM(EVT_CMD_STATUS)                             /* 0x0F */
    CASE_PRINT_ENUM(EVT_HARDWARE_ERROR)                         /* 0x10 */
    CASE_PRINT_ENUM(EVT_NUM_COMP_PKTS)                          /* 0x13 */
    CASE_PRINT_ENUM(EVT_DATA_BUFFER_OVERFLOW)                   /* 0x1A */
    CASE_PRINT_ENUM(EVT_ENCRYPTION_KEY_REFRESH_COMPLETE)        /* 0x30 */
    case EVT_LE_META_EVENT:                                     /* 0x3E */
      DBMSG(DBL_ERRORS, "EVT_LE_META_EVENT")
      report_event_pckt = (evt_le_meta_event *) event_pckt->data;
      handled = display_meta_event(report_event_pckt);
      break;
    case EVT_VENDOR:
      evt_blue = (evt_blue_aci *) (event_pckt->data);
      DBPR(DBL_HCI_EVENTS, evt_blue->ecode, "%04X", "ecode for HCI events")
      handled = display_ecode(evt_blue);
      break;  /* EVT_VENDOR case */    
    default: 
      handled = false;
      DBPR(DBL_ERRORS, event_pckt->evt, "%02X", "Unknown event received")
  }/* switch(event_pckt->evt)*/
  return handled;
}

bool display_meta_event(evt_le_meta_event *report_event_pckt) {
  bool handled = true;
  switch(report_event_pckt->subevent) {
    CASE_PRINT_ENUM(EVT_LE_CONN_COMPLETE)                       /* 0x01 */
    CASE_PRINT_ENUM(EVT_LE_ADVERTISING_REPORT)                  /* 0x02 */
    CASE_PRINT_ENUM(EVT_LE_CONN_UPDATE_COMPLETE)                /* 0x03 */
    CASE_PRINT_ENUM(EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE)  /* 0x04 */
    CASE_PRINT_ENUM(EVT_LE_LTK_REQUEST)                         /* 0x05 */
    default:
      handled = false; 
      DBPR(DBL_ERRORS, report_event_pckt->subevent, "%02X", "Unknown subevent received")
  }/* switch(event_pckt->evt)*/
  return handled;
}

void display_procedure_complete(evt_blue_aci *evt_blue) {
  evt_gap_procedure_complete * procedure_complete_pckt = (evt_gap_procedure_complete *) evt_blue->data;
  switch (procedure_complete_pckt->procedure_code) {
    case GAP_LIMITED_DISCOVERY_PROC:                  DBMSG(DBL_ERRORS, "GAP_LIMITED_DISCOVERY_PROC complete") break;
    case GAP_GENERAL_DISCOVERY_PROC:                  DBMSG(DBL_ERRORS, "GAP_GENERAL_DISCOVERY_PROC complete") break;
    case GAP_NAME_DISCOVERY_PROC:                     DBMSG(DBL_ERRORS, "GAP_NAME_DISCOVERY_PROC complete") break;
    case GAP_AUTO_CONNECTION_ESTABLISHMENT_PROC:      DBMSG(DBL_ERRORS, "GAP_AUTO_CONNECTION_ESTABLISHMENT_PROC complete") break;
    case GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC:   DBMSG(DBL_ERRORS, "GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC complete") break;
    case GAP_SELECTIVE_CONNECTION_ESTABLISHMENT_PROC: DBMSG(DBL_ERRORS, "GAP_SELECTIVE_CONNECTION_ESTABLISHMENT_PROC complete") break;
    case GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC:    DBMSG(DBL_ERRORS, "GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC complete") break;
    case GAP_OBSERVATION_PROC_IDB05A1:                DBMSG(DBL_ERRORS, "GAP_OBSERVATION_PROC_IDB05A1 complete") break;
    default: DBPR(DBL_ERRORS, procedure_complete_pckt->procedure_code, "02X", "unknown procedure complete code") break;
  }
  DBPR(DBL_ERRORS, procedure_complete_pckt->status, "%02X", "status code from procedure complete")
  DBBUFF(DBL_ERRORS, procedure_complete_pckt->data)
}

bool display_ecode(evt_blue_aci *evt_blue) {
  evt_hal_events_lost_IDB05A1 *events_lost;
  bool handled = true;
  switch (evt_blue->ecode) {
    /* GAP EVENTS: see comments in bluenrg_gap_aci.h for any of the following */
    CASE_PRINT_ENUM(EVT_BLUE_GAP_LIMITED_DISCOVERABLE) 
    CASE_PRINT_ENUM(EVT_BLUE_GAP_PAIRING_CMPLT) 
    CASE_PRINT_ENUM(EVT_BLUE_GAP_PASS_KEY_REQUEST) 
    CASE_PRINT_ENUM(EVT_BLUE_GAP_AUTHORIZATION_REQUEST) 
    CASE_PRINT_ENUM(EVT_BLUE_GAP_SLAVE_SECURITY_INITIATED) 
    CASE_PRINT_ENUM(EVT_BLUE_GAP_BOND_LOST) 
    CASE_PRINT_ENUM(EVT_BLUE_GAP_DEVICE_FOUND) 
    case EVT_BLUE_GAP_PROCEDURE_COMPLETE:
      DBMSG(DBL_DECODED_EVENTS, "EVT_BLUE_GAP_PROCEDURE_COMPLETE")
      display_procedure_complete(evt_blue);
      break;
    CASE_PRINT_ENUM(EVT_BLUE_GAP_ADDR_NOT_RESOLVED_IDB05A1) 
    //CASE_PRINT_ENUM(EVT_BLUE_GAP_RECONNECTION_ADDRESS_IDB04A1) 
    /* GATT EVENTS) see comments in bluenrg_gatt_aci.h for any of the following */
    CASE_PRINT_ENUM(EVT_BLUE_GATT_ATTRIBUTE_MODIFIED) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_PROCEDURE_TIMEOUT) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_EXCHANGE_MTU_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_FIND_INFORMATION_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_READ_BY_TYPE_RESP)
    CASE_PRINT_ENUM(EVT_BLUE_ATT_READ_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_READ_BLOB_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_READ_MULTIPLE_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_PREPARE_WRITE_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_ATT_EXEC_WRITE_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_INDICATION) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_NOTIFICATION) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_PROCEDURE_COMPLETE) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_ERROR_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_WRITE_PERMIT_REQ) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_READ_PERMIT_REQ) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_READ_MULTI_PERMIT_REQ) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_TX_POOL_AVAILABLE) 
    CASE_PRINT_ENUM(EVT_BLUE_GATT_PREPARE_WRITE_PERMIT_REQ) 
    /* HAL EVENTS: see comments in bluenrg_hal_aci.h for any of the following */
    case EVT_BLUE_HAL_EVENTS_LOST_IDB05A1: {
      events_lost = (evt_hal_events_lost_IDB05A1 *) (evt_blue->data);
      if (DBLVL >= DBL_ERRORS) {
        DBMSG(DBL_ERRORS, "************************ Received LOST events event. **************************")
        DBPR8(DBL_ERRORS, events_lost->lost_events, "Here is the (little endian) bit mask:")
      }
    }   break;   /* EVT_BLUE_HAL_EVENTS_LOST_IDB05A1 */
    CASE_PRINT_ENUM(EVT_BLUE_HAL_CRASH_INFO_IDB05A1) 
    /* L2CAP EVENTS: see comments in bluenrg_l2cap_aci.h for any of the following */
    CASE_PRINT_ENUM(EVT_BLUE_L2CAP_CONN_UPD_RESP) 
    CASE_PRINT_ENUM(EVT_BLUE_L2CAP_PROCEDURE_TIMEOUT) 
    CASE_PRINT_ENUM(EVT_BLUE_L2CAP_CONN_UPD_REQ) 
    /* UPDATER EVENTS: see comments in bluenrg_updater_aci.h for any of the following */
    CASE_PRINT_ENUM(EVT_BLUE_INITIALIZED) 
    default:
      DBMSG(DBL_ERRORS, "*** Unknown blue event ecode")
      handled = false;
  } /* ecode switch */ 
  return handled;
} /* display_ecode */

/* process BlueRNG-MS vendor codes. The two byte vendor ecode is composed of 
 *  a 6 bit event group id (egid) and a 10 bit event id (eid).
 *  See ST's UM1865 document for more information.
 */
bool display_ecode(uint8_t hb, uint8_t lb) {
  uint8_t egid = hb >> 2;
  uint16_t eid = (lb << 2) + (hb & 0x03);
  bool displayed = true;
  DBPR(DBL_ALL_BLE_EVENTS, egid,"%02X", "processing ecode - group ID")
  DBPR(DBL_ALL_BLE_EVENTS, eid, "%02X", "processing ecode - event ID")
  switch(egid) {
     case 0:  /* HCI */
        switch(eid) {
          default: 
            DBMSG(DBL_ERRORS, "*** received HCI group ecode not recognized")
            displayed = false;
        }
        break;
     case 1:  /* GAP */  
       switch(eid) {
         case 0: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Gap_Limited_Discoverable event")  break;
         case 1: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Gap_Pairing_Complete event")      break;
         case 2: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Pass_Key_Request event")          break;
         case 3: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Authorization_Request event")     break;
         case 4: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Slave_Security_Initiated event")  break;
         case 5: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Gap_Bond_Lost event")             break;
         case 7: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Gap_Procedure_Complete event")    break;
         case 8: DBMSG(DBL_ERRORS,  "*** unhandled Evt_Blue_Gap_Addr_Not_Resolved event")     break;
         case 13: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gap_Addr_Not_Resolved event")     break;
         default: 
           DBMSG(DBL_ERRORS, "*** received GAP group ecode not recognized")
           displayed = false;
       }
       break;
     case 2:  /* L2CAP */
       switch(eid) {
        case 0: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_L2CAP_Connection_Update_Resp  event")   break;
        case 1: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_L2CAP_Procedure_Timeout event")         break;
        case 3: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_L2CAP_Connection_Update_Request event") break;
        default: 
            DBMSG(DBL_ERRORS, "*** received L2CAP group ecode not recognized\n")
            displayed = false;
       }
       break;
     case 3:   /* GATT */
       switch(eid) {
         case  1: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Attribute_modified  event")   break;
         case  2: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Procedure_Timeout  event")   break;
         case  3: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Exchange_MTU_Resp  event")   break;
         case  4: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Find_Information_Resp  event")   break;
         case  5: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Find_By_Type_Value_Resp  event")   break;
         case  6: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Read_By_Type_Resp  event")   break;
         case  7: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Read_Resp  event")   break;
         case  8: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Read_Blob_Resp  event")   break;
         case  9: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Read_Multiple_Resp  event")   break;
         case 10: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Read_By_Group_Type_Resp  event")   break;
         case 12: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Prepare_Write_Resp  event")   break;
         case 13: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Att_Exec_Write_Resp  event")   break;
         case 14: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Indication  event")   break;
         case 15: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_notification  event")   break;
         case 16: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Procedure_Complete  event")   break;
         case 17: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Error_Response  event")   break;
         case 18: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Disc_Read_Charac_By_UUID_Resp  event")   break;
         case 19: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Write_Permit_req  event")   break;
         case 20: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Read_Permit_Req  event")   break;
         case 21: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Read_Multi_Permit_Req  event")   break;
         case 22: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Tx_Pool_Available  event")   break;
         case 23: DBMSG(DBL_ERRORS, "*** unhandled Evt_Blue_Gatt_Server_Confirmation  event")   break;
         default: 
           DBMSG(DBL_ERRORS, "*** received GATT group ecode not recognized")
           displayed = false;
       }
       break;
     default:
       DBMSG(DBL_ERRORS, "*** received vender event group code not recognized")
       displayed = false;
  }
  return displayed;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// display explanation of various return codes (see ble_status.h for more details)
//
////////////////////////////////////////////////////////////////////////////////////////////////

void hci_print_ret(tBleStatus ret) {
  switch (ret) {
    CASE_PRINT_ENUM(BLE_STATUS_SUCCESS)                          /* (0x00) */
    CASE_PRINT_ENUM(ERR_UNKNOWN_HCI_COMMAND)                     /* (0x01) */
    CASE_PRINT_ENUM(ERR_UNKNOWN_CONN_IDENTIFIER)                 /* (0x02) */
    CASE_PRINT_ENUM(ERR_AUTH_FAILURE)                            /* (0x05) */
    CASE_PRINT_ENUM(ERR_PIN_OR_KEY_MISSING)                      /* (0x06) */
    CASE_PRINT_ENUM(ERR_MEM_CAPACITY_EXCEEDED)                   /* (0x07) */
    CASE_PRINT_ENUM(ERR_CONNECTION_TIMEOUT)                      /* (0x08) */
    CASE_PRINT_ENUM(ERR_COMMAND_DISALLOWED)                      /* (0x0C) */
    CASE_PRINT_ENUM(ERR_UNSUPPORTED_FEATURE)                     /* (0x11) */
    CASE_PRINT_ENUM(ERR_INVALID_HCI_CMD_PARAMS)                  /* (0x12) */
    CASE_PRINT_ENUM(ERR_RMT_USR_TERM_CONN)                       /* (0x13) */
    CASE_PRINT_ENUM(ERR_RMT_DEV_TERM_CONN_LOW_RESRCES)           /* (0x14) */
    CASE_PRINT_ENUM(ERR_RMT_DEV_TERM_CONN_POWER_OFF)             /* (0x15) */
    CASE_PRINT_ENUM(ERR_LOCAL_HOST_TERM_CONN)                    /* (0x16) */
    CASE_PRINT_ENUM(ERR_UNSUPP_RMT_FEATURE)                      /* (0x1A) */
    CASE_PRINT_ENUM(ERR_INVALID_LMP_PARAM)                       /* (0x1E) */
    CASE_PRINT_ENUM(ERR_UNSPECIFIED_ERROR)                       /* (0x1F) */
    CASE_PRINT_ENUM(ERR_LL_RESP_TIMEOUT)                         /* (0x22) */
    CASE_PRINT_ENUM(ERR_LMP_PDU_NOT_ALLOWED)                     /* (0x24) */
    CASE_PRINT_ENUM(ERR_INSTANT_PASSED)                          /* (0x28) */
    CASE_PRINT_ENUM(ERR_PAIR_UNIT_KEY_NOT_SUPP)                  /* (0x29) */
    CASE_PRINT_ENUM(ERR_CONTROLLER_BUSY)                         /* (0x3A) */
    CASE_PRINT_ENUM(ERR_DIRECTED_ADV_TIMEOUT)                    /* (0x3C) */
    CASE_PRINT_ENUM(ERR_CONN_END_WITH_MIC_FAILURE)               /* (0x3D) */
    CASE_PRINT_ENUM(ERR_CONN_FAILED_TO_ESTABLISH)                /* (0x3E) */
    CASE_PRINT_ENUM(BLE_STATUS_FAILED)                           /* (0x41) The command cannot be executed due to the current state of the device. */
    CASE_PRINT_ENUM(BLE_STATUS_INVALID_PARAMS)                   /* (0x42) */
    CASE_PRINT_ENUM(BLE_STATUS_NOT_ALLOWED)                      /* (0x46) Not allowed to start the procedure (e.g. another the procedure is ongoing or cannot be started on the given handle)*/
    CASE_PRINT_ENUM(BLE_STATUS_ERROR)                            /* (0x47) */
    CASE_PRINT_ENUM(BLE_STATUS_ADDR_NOT_RESOLVED)                /* (0x48) */
    CASE_PRINT_ENUM(FLASH_READ_FAILED)                           /* (0x49) */
    CASE_PRINT_ENUM(FLASH_WRITE_FAILED)                          /* (0x4A) */
    CASE_PRINT_ENUM(FLASH_ERASE_FAILED)                          /* (0x4B) */
    CASE_PRINT_ENUM(BLE_STATUS_INVALID_CID)                      /* (0x50) */
    CASE_PRINT_ENUM(TIMER_NOT_VALID_LAYER)                       /* (0x54) */
    CASE_PRINT_ENUM(TIMER_INSUFFICIENT_RESOURCES)                /* (0x55) */
    CASE_PRINT_ENUM(BLE_STATUS_CSRK_NOT_FOUND)                   /* (0x5A) */
    CASE_PRINT_ENUM(BLE_STATUS_IRK_NOT_FOUND)                    /* (0x5B) */
    CASE_PRINT_ENUM(BLE_STATUS_DEV_NOT_FOUND_IN_DB)              /* (0x5C) */
    CASE_PRINT_ENUM(BLE_STATUS_SEC_DB_FULL)                      /* (0x5D) */
    CASE_PRINT_ENUM(BLE_STATUS_DEV_NOT_BONDED)                   /* (0x5E) */
    CASE_PRINT_ENUM(BLE_STATUS_DEV_IN_BLACKLIST)                 /* (0x5F) */
    CASE_PRINT_ENUM(BLE_STATUS_INVALID_HANDLE)                   /* (0x60) */
    CASE_PRINT_ENUM(BLE_STATUS_INVALID_PARAMETER)                /* (0x61) */
    CASE_PRINT_ENUM(BLE_STATUS_OUT_OF_HANDLE)                    /* (0x62) */
    CASE_PRINT_ENUM(BLE_STATUS_INVALID_OPERATION)                /* (0x63) */
    CASE_PRINT_ENUM(BLE_STATUS_INSUFFICIENT_RESOURCES)           /* (0x64) */
    CASE_PRINT_ENUM(BLE_INSUFFICIENT_ENC_KEYSIZE)                /* (0x65) */
    CASE_PRINT_ENUM(BLE_STATUS_CHARAC_ALREADY_EXISTS)            /* (0x66) */
    CASE_PRINT_ENUM(BLE_STATUS_NO_VALID_SLOT)                    /* (0x82) */
    CASE_PRINT_ENUM(BLE_STATUS_SCAN_WINDOW_SHORT)                /* (0x83) */
    CASE_PRINT_ENUM(BLE_STATUS_NEW_INTERVAL_FAILED)              /* (0x84) */
    CASE_PRINT_ENUM(BLE_STATUS_INTERVAL_TOO_LARGE)               /* (0x85) */
    CASE_PRINT_ENUM(BLE_STATUS_LENGTH_FAILED)                    /* (0x86) */
    CASE_PRINT_ENUM(BLE_STATUS_TIMEOUT)                          /* (0xFF) */
    CASE_PRINT_ENUM(BLE_STATUS_PROFILE_ALREADY_INITIALIZED)      /* (0xF0) */
    CASE_PRINT_ENUM(BLE_STATUS_NULL_PARAM)                       /* (0xF1)  */
    default: DBPR(DBL_ERRORS, ret,"%02X", "Unknown return code")
  }
}
