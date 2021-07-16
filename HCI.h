/*!
 * @file /HCI.h
 * @brief A collection of common operations for BlueNRG HCI.
 * @details
 * A key resource provided is global handling of all BlueRNG events. By adding the following as global expectations, you get pretty comprehensive error checking:
 *     expect_globally_condition(check_initialization_or_reset, NO_ACTION, NO_ARGS);
 *     expect_globally_condition(check_event, NO_ACTION, NO_ARGS);
 * However, in most cases, all you get is debug information about that unexpected event so you can deal with it appropriately.
 * 
 * Also provided are a few things to set up BlueRNG. Currently these are geared toward a client/central but to start BlueRNG for a client, you just need a step
 * in your protocol like this:
 *     PERFORM(start_HCI, NULL);
 *       expect(reset_reason, SPECIFICALLY(RESET_NORMAL), AND_DO(set_MAC_addr_action), WITH(NO_ARGS));
 * This will start HCI, checking for a successful start, and setting the default MAC address for your device.
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HCI_H
#define HCI_H
#include "production.h"

#define OUR_MAC_ADDR {0x12, 0x34, 0x00, 0xE1, 0x80, 0x02}
#define OUR_DEVICE_NAME "BlueNRG-MS"

bool start_HCI(void * args);

void set_public_MAC_addr();
const char * get_device_name();
bool set_MAC_addr_action(hci_event_pckt *event_pckt, DUMMY_ARG);  // match action signature by adding unused event_pckt;

void hci_print_ret(tBleStatus ret);

// global event condition functions to just display the event

bool check_initialization_or_reset(hci_event_pckt *event_pckt);
bool check_event(hci_event_pckt *event_pckt);

bool display_initialization_or_reset(hci_event_pckt *event_pckt, DUMMY_ARG);
bool display_event(hci_event_pckt *event_pckt, DUMMY_ARG);

#endif
