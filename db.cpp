/*!
 * @file db.h
 * @brief Implementation of a simple database of services and characteristics of devices found. 
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



#include "db.h"
#include "dbprint.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// device info database
//
////////////////////////////////////////////////////////////////////////////////////////////////

static db_record_t device_db[MAX_RECORDS];
static int num_records = 0;
static int parent = 0;

void init_device_db() {
  num_records = 0;
}

void set_context(attribute_context_t * context, db_type dbtype, int parent, uint16_t connection_handle) {
  context->dbtype = dbtype;
  context->parent = parent;
  context->connection_handle = connection_handle;
}

/*
 * adding records and enumeration both assume the following about how things added to the db
 * 1) a device is added
 * 2) then its primary services are added
 * 3) for each primary service, included services are added
 * 4) for each service (primary or included), characteristics are added
 * 5) then another device type is added and 2-4 are repeated
 * 
 */

int add_device_to_device_db(tBDAddr * device_addr) {
  if (num_records == MAX_RECORDS) {
    PRINTF("can't add more db entries, DB is full\n")
    return -1;
  }
  else {
    device_db[num_records].context.dbtype = db_device;
    copy_addr(*device_addr, &(device_db[num_records].dora.addr));
    device_db[num_records].processed = false;
    num_records++;  
    return num_records-1;
  }
}

db_record_t * new_entry_in_device_db() {
  if (num_records == MAX_RECORDS) {
    PRINTF("can't add more db entries, DB is full\n")
    return NULL;
  }
  else {
    num_records++;  
    return &(device_db[num_records-1]);  
  }
}

void put_back_entry_in_device_db() {
  num_records--; 
}


void copy_attribute_context(attribute_context_t * from, attribute_context_t * to) {
  to->dbtype = from->dbtype;
  to->parent = from->parent;
  to->connection_handle = from->connection_handle;  
}

int add_attribute_to_device_db(attribute_info_t * attribute, attribute_context_t context) {
  int i, size;
  if (num_records == MAX_RECORDS) {
    PRINTF("can't add more db entries, DB is full\n")
    return -1;
  }
  else {
    PRINTF("adding "); print_uuid(&(attribute->uuid)); PRINTF("\n");
    device_db[num_records].context.dbtype = context.dbtype;
    device_db[num_records].context.parent = context.parent;
    device_db[num_records].context.connection_handle = context.connection_handle;
    device_db[num_records].dora.attr.starting_handle = attribute->starting_handle;
    device_db[num_records].dora.attr.ending_handle = attribute->ending_handle;
    device_db[num_records].dora.attr.connection_handle = attribute->connection_handle;
    device_db[num_records].dora.attr.uuid.is_16_bit = attribute->uuid.is_16_bit;
    if (attribute->uuid.is_16_bit) size = 2;
    else size = 16;
    for (i=0; i<size; i++) device_db[num_records].dora.attr.uuid.bytes[i] = attribute->uuid.bytes[i];
    device_db[num_records].processed = false;
    num_records++;  
    return num_records-1;
  }
}

/*
 * functions to get data from the DB
 */

void mark_processed_in_device_db(int index) { device_db[index].processed = true; }

attribute_info_t * get_attribute_info_from_device_db(int index) { return &(device_db[index].dora.attr); }

handle_value_pair_t * get_handle_value_pair_from_device_db(int index) {return &(device_db[index].dora.handle_value_pair);}


int last_entry_for_device_in_device_db(int device_index) {
  int i;
  for (i=device_index+1; i<num_records; i++) {
    if ( device_db[i].context.dbtype == db_device ) return i-1; 
  }
  return -1;
}

int recall_first_unprocessed_in_device_db() {
  int i;
  for (i=0; i<num_records; i++) {
    if (  !(device_db[i].processed) ) return i;
  }
  return -1;
}
  
int  recall_first_unprocessed_of_type_in_device_db(db_type dbtype) {
  int i;
  for (i=0; i<num_records; i++) {
    if ( (device_db[i].context.dbtype == dbtype) && !(device_db[i].processed) ) return i;
  }
  return -1;
}

int recall_first_unprocessed_of_type_for_device_in_device_db(db_type dbtype, int device_index) {
  int i, num;
  for (i=device_index+1; (i<num_records) && (device_db[i].context.dbtype != db_device); i++) {
    if ( (device_db[i].context.dbtype == dbtype) && !(device_db[i].processed) ) return i;
  }
  return -1;
}

int num_unprocessed_in_device_db() {
  int i, num;
  num = 0;
  for (i=0; i<num_records; i++) {
    if ( !(device_db[i].processed) ) num++;
  }
  return num;
}

int num_unprocessed_of_type_in_device_db(db_type dbtype) {
  int i, num;
  num = 0;
  for (i=0; i<num_records; i++) {
    if ( (device_db[i].context.dbtype == dbtype) && !(device_db[i].processed) ) num++;
  }
  return num;
}

int num_unprocessed_of_type_for_device_in_device_db(db_type dbtype, int device_index) {
  int i, num;
  num = 0;
  for (i=device_index+1; (i<num_records) && (device_db[i].context.dbtype != db_device); i++) {
    if ( (device_db[i].context.dbtype == dbtype) && !(device_db[i].processed) ) num++;
  }
  return num;
}

void mark_unprocessed_in_device_db() {
  int i;
  for (i=0; i<num_records; i++) {
    device_db[i].processed = false;
  }
}
  
void mark_unprocessed_of_type_in_device_db(db_type dbtype) {
  int i;
  for (i=0; i<num_records; i++) {
    if ( (device_db[i].context.dbtype == dbtype) )device_db[i].processed = false;
  }
}
  
void mark_unprocessed_of_type_for_device_in_device_db(db_type dbtype, int device_index) {
  int i;
  for (i=device_index+1; (i<num_records) && (device_db[i].context.dbtype != db_device); i++) {
    if ( (device_db[i].context.dbtype == dbtype) )device_db[i].processed = false;
  }
}

/*
 * functions for traversing everything in the DB
 */

// good idiom for using next___ is next___(&index, index) to start from where the last place this type was found and return the next place it is found

#define FIRST_DEVICE -1

bool device_db_next_device(int * index, int starting) {
  int i;
  if (starting == -1 && (device_db[0].context.dbtype == db_device)) {
    *index = 0;
    return true;
  }
  for (i = starting+1; i < num_records; i++) {
    if (device_db[i].context.dbtype == db_device) {
      *index = i;
      return true;
    }
  }
  return false;
}

int device_db_last_device_record(int starting) {
  int i;
  for (i = starting+1; i < num_records; i++) {
    if (device_db[i].context.dbtype == db_device) return (i-1);
  }
  return (num_records - 1);
}

bool device_db_next_primary_service(int *index, int starting, int ending) {
  int i;
  for (i = starting+1; i < ending; i++) {
    if (device_db[i].context.dbtype == db_primary_service) {
      *index = i;
      return true;
    }
  }
  return false;
}

// parent_index should be the index of the primary service this is part of
bool device_db_next_included_service(int *index, int starting, int ending, int parent_index) {
  int i;
  for (i = starting+1; i < ending; i++) {
    if ((device_db[i].context.dbtype == db_included_service) && (device_db[i].context.parent == parent_index)) {
      *index = i;
      return true;
    }
  }
  return false;
}

// parent_index should be the index of the service this is part of
bool device_db_next_characteristic (int *index, int starting, int ending, int parent_index) {
  int i;
  for (i = starting+1; i < ending; i++) {
    if ((device_db[i].context.dbtype == db_characteristic) && (device_db[i].context.parent == parent_index)) {
      *index = i;
      return true;
    }
  }
  return false;
}

/* 
 *  enumerate as follows:
 *  1) go device by device, for each device
 *     1) print the device address
 *     2) go primary service by primary service, for each primary service
 *        1) print the handle, and uuid, and handle range
 *        2) go included service by included service, for each included service
 *           1) print the service handle, uuid, and handle range
 *           2) go characteristic by characteristic, for each characteristic, print the characteristic uuid and type
 *        3) go characteristic by characcteristic, for each characteristic of the primary servicce, print the characteristic uuid and type 
 */

 void indent(int n) {
  int i;
  //PRINTF("indention: %d", n);
  for (i=0; i<n; i++) PRINTF(" ");
 }

#define INDENTION_INCREASE 3

 void print_device_db() {
  int indention;
  int device_index;
  int device_last_index;
  int primary_service_index;
  int included_service_index;
  int characteristic_index;
  device_index = FIRST_DEVICE;
  indention = 0;
  while (device_db_next_device(&device_index, device_index)) {
    PRINTF("device: ");
    print_addr(device_db[device_index].dora.addr);
    PRINTF("\n");
    device_last_index = device_db_last_device_record(device_index);
    // enumerate primary services 
    primary_service_index = device_index; /* start looking here */
    indention += INDENTION_INCREASE;
    while(device_db_next_primary_service(&primary_service_index, primary_service_index, device_last_index)) {
      indent(indention);
      PRINTF("primary_service(%d) last record (%d) ", primary_service_index, device_last_index);
      print_uuid(&(device_db[primary_service_index].dora.attr.uuid));
      PRINTF("\n");
      // enumerate characteristics of the primary service
      characteristic_index = primary_service_index;
      indention += INDENTION_INCREASE;
      while (device_db_next_characteristic(&characteristic_index, characteristic_index, device_last_index, primary_service_index)) {
        indent(indention);
        PRINTF("characteristic: ");
        print_uuid(&(device_db[characteristic_index].dora.attr.uuid));
        PRINTF("\n");
      }
      indention -= INDENTION_INCREASE;
      // enumerate included services and their characteristics
      included_service_index = primary_service_index;
      indention += INDENTION_INCREASE;
      while (device_db_next_included_service(&included_service_index, included_service_index, device_last_index, primary_service_index)) {
        indent(indention);
        PRINTF("included_service: ");
        print_uuid(&(device_db[included_service_index].dora.attr.uuid));
        PRINTF("\n");
        indention += INDENTION_INCREASE;
        characteristic_index = included_service_index;
        indention += INDENTION_INCREASE;
        while (device_db_next_characteristic(&characteristic_index, characteristic_index, device_last_index, included_service_index)) {
          indent(indention);
          PRINTF("characteristic: ");
          PRINTF("handle: %04X ", device_db[characteristic_index].dora.handle_value_pair.handle)
          PRINTF("value: ")
          for (int i=0; i < device_db[characteristic_index].dora.handle_value_pair.len-1; i++) {
            PRINTF("02X:", device_db[characteristic_index].dora.handle_value_pair.value[i])
          }
          PRINTF("02X:", device_db[characteristic_index].dora.handle_value_pair.value[device_db[characteristic_index].dora.handle_value_pair.len-1])
          PRINTF("\n");
        }
        indention -= INDENTION_INCREASE;
      }
      indention -= INDENTION_INCREASE;
    }
    indention -= INDENTION_INCREASE;
  }
}

void dump_device_db() {
  int i;
  for (i=0; i < num_records; i++) {
    PRINTF("%02d %02d %02d \n", i, device_db[i].context.dbtype, device_db[i].context.parent)
  }
}

// The following can be used as an action to perform to populate the db with info from the attribute info in an event response
bool add_device_db_entry_from_event(hci_event_pckt *event_pckt, arg_t context_arg) {
  evt_blue_aci *evt_blue;
  evt_att_read_by_group_resp * read_by_group_type_response;
  evt_att_read_by_type_resp * read_by_type_resp;
  db_record_t * new_db_record;
  attribute_context_t * context = (attribute_context_t *) context_arg;
  int i;
  if (event_pckt->evt == EVT_VENDOR) {
    evt_blue = (evt_blue_aci *) (event_pckt->data);
    switch(evt_blue->ecode) {
      case EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP:
        read_by_group_type_response = (evt_att_read_by_group_resp *) (evt_blue->data); 
        DBPR(DBL_DECODED_EVENTS, read_by_group_type_response->conn_handle, "%d", "read_by_group_type_response for handle")
        print_attr_list(read_by_group_type_response->attribute_data_list, read_by_group_type_response->event_data_length, read_by_group_type_response->attribute_data_length);
        // for each attribute in the attribute list returned, add it to device_db as a primary service
        for (i = 0; i < read_by_group_type_response->event_data_length; i += read_by_group_type_response->attribute_data_length) {
          new_db_record = new_entry_in_device_db();
          get_attribute_info(read_by_group_type_response->attribute_data_list + i, read_by_group_type_response->attribute_data_length, &(new_db_record->dora.attr));
          // skip adding this if handle range is invalid (TODO: understand why this happens)
          if (new_db_record->dora.attr.starting_handle > new_db_record->dora.attr.ending_handle) put_back_entry_in_device_db();
          else {
            new_db_record->dora.attr.connection_handle = context->connection_handle;
            copy_attribute_context(context, &(new_db_record->context));
          }
        }
        return true;
      case EVT_BLUE_ATT_READ_BY_TYPE_RESP:
        read_by_type_resp = (evt_att_read_by_type_resp *) (evt_blue->data);
        DBPR(DBL_DECODED_EVENTS, read_by_type_resp->conn_handle, "%d", "evt_att_read_by_type_resp for handle")
        print_attr_list(read_by_type_resp->handle_value_pair, read_by_type_resp->event_data_length, read_by_type_resp->handle_value_pair_length);
        for (i = 0; i < read_by_type_resp->event_data_length; i += read_by_type_resp->handle_value_pair_length) {
          new_db_record = new_entry_in_device_db();
          get_handle_value_pair(read_by_type_resp->handle_value_pair + i, read_by_type_resp->handle_value_pair_length, &(new_db_record->dora.handle_value_pair));
          new_db_record->dora.handle_value_pair.connection_handle = context->connection_handle;
          copy_attribute_context(context, &(new_db_record->context));
        }
        return true;
      default:
        PRINTF("print service discovered called on wrong event type\n");
        return false;      
    };
  }
  PRINTF("print service discovered called on wrong type\n");
  return false;
}
