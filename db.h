/*!
 * @file db.h
 * @brief Provides a simple database of services and characteristics of devices found. 
 * @details
 * The database contains heterogeneous information for devices, services, and characteristics. Since each has their only different type of data, a union type
 * called "dora" (device or attribute) is used. However, everything has some context information that provides information about the connection handle used
 * to find that information, the device or attribute that this was found under (i.e., the service or attribute parent index), and the type of thing being
 * enumerated when this was found (primary service, included service, characteristic) - this indicating the type of item being added to the db. Also, each
 * entry can track the notion of whether it has been used/processed for something so that you can go through the db and do something for each type of service
 * or attribute and check that you have done that for them all. So a db record has a dora and context information. The connection handle is duplicated in both
 * dora and context because things that only use attribute information need to know the connection handle and things that only look at context information need it too.
 * 
 * There are many ways to add to and to query the database, probably way more than typically needed. The most useful ones are:
 * - add_device_to_device_db - adds the given device address information to the db and returns an index 
 * - new_entry_in_device_db  - just returns a pointer to the new record that you can fill in manually
 * - add_device_db_entry_from_event - can be used as an action when an expectation is met to automatically add that to the db, note that you still need to fill in the
 * context information and pass this as an argument to the action 
 * - RECALL_DEVICE - to get the next (unprocessed) device from the db
 * - RECALL_PRIMARY_SERVICE - to get the next (unprocessed) primary service for a given device
 * - PRIMARY_SERVICES_TODO - check to see if there are unprocessed primary services for a given device
 * - RESET_ALL_PRIMARY_SERVICES - mark all primary services unprocessed so you can enumerate them again for a different purpose
 * - print_db - print out all information in the device db, hierarchically by device, service, then attribute
 * 
 * Note included services are considered. Whether these get added to the db or not is up to the calling application, but if they are added, then
 * they are printed and available, along with their characteristics. This has not been fully tested as I've not seen anything with included services to test it with.
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DB_H
#define DB_H

#include <stdint.h>
#include "addrs.h"
#include "get_data.h"

#define MAX_RECORDS 500

/*
 * A db entry is either a device or an attribute.
 * If it is a device then the record contains just the device address.
 * For anything else, the record contains the attribute info and the attribute context.
 * To avoid wasting 6 bytes of space for each attribute to hold the device address that it doesn't need, a union is used to share this with the attribute info space.
 * An alternative would be to have a separate DB for devices, but it is probably better to keep everything in one DB.
 * The notion to capture this is a "dora" which is either a device or some type of attribute in the device's GATT.
 */

typedef enum {db_device, db_primary_service, db_included_service, db_characteristic} db_type;

typedef union dora_u {
  tBDAddr addr;
  attribute_info_t attr;
  handle_value_pair_t handle_value_pair;
} dora_t;

typedef struct attribute_context_s {
  db_type dbtype;
  int parent;
  uint16_t connection_handle;
} attribute_context_t;

void set_context(attribute_context_t * context, db_type dbtype, int parent, uint16_t connection_handle);

typedef struct db_record_s {
  dora_t dora;
  attribute_context_t context;
  bool processed;
} db_record_t;

// both add device and add attribute return the index of the entry just added; this is useful for populating the parent index in future additions

int add_device_to_device_db(tBDAddr * device_addr);

int last_entry_for_device_in_device_db(int device_index);

// The following can be used as an action to perform to populate the db with info from the attribute info in an event response
bool add_device_db_entry_from_event(hci_event_pckt *event_pckt, arg_t context_arg);

db_record_t * new_entry_in_device_db(); // just returns a pointer to a new DB entry that the client can fill out
void put_back_entry_in_device_db();     // when decide not to use this entry (kind of like pop)
void copy_attribute_context(attribute_context_t * from, attribute_context_t * to);

attribute_info_t * get_attribute_info_from_device_db(int index);
handle_value_pair_t * get_handle_value_pair_from_device_db(int index);

int add_attribute_to_device_db(attribute_info_t * attribute, attribute_context_t context);

void print_device_db();
void dump_device_db();

void mark_processed_in_device_db(int index);

int recall_first_unprocessed_in_device_db();
int recall_first_unprocessed_of_type_in_device_db(db_type dbtype);
int recall_first_unprocessed_of_type_for_device_in_device_db(db_type dbtype, int device_index);

int num_unprocessed_in_device_db();
int num_unprocessed_of_type_in_device_db(db_type dbtype, int device_index);
int num_unprocessed_of_type_for_device_in_device_db(db_type dbtype, int device_index);

void mark_unprocessed_in_device_db();
void mark_unprocessed_of_type_in_device_db(db_type dbtype);
void mark_unprocessed_of_type_for_device_in_device_db(db_type dbtype, int device_index);

// some syntactic sugar for more self-documenting code
#define RECALL_DEVICE recall_first_unprocessed_in_device_db();
#define RECALL_PRIMARY_SERVICE(device_index) recall_first_unprocessed_of_type_for_device_in_device_db(db_primary_service, device_index);
#define RECALL_INCLUDED_SERVICE(device_index) recall_first_unprocessed_of_type_for_device_in_device_db(db_included_service, device_index);
#define RECALL_SERVICE(device_index) recall_first_unprocessed_of_type_for_device_in_device_db(db_primary_service, device_index); 
#define PRIMARY_SERVICES_TODO(device_index) (num_unprocessed_of_type_for_device_in_device_db(db_primary_service, device_index) != 0);
#define INCLUDED_SERVICES_TODO(device_index) (num_unprocessed_of_type_for_device_in_device_db(db_included_service, device_index) != 0);
#define RESET_ALL_PRIMARY_SERVICES(device_index) mark_unprocessed_of_type_for_device_in_device_db(db_primary_service, device_index);

#define PARENT(x) x
 


#endif
