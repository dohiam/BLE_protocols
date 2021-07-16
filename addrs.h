/*!
 * @file addrs.h
 * @brief Implementation of a simple database of devices found
 * @details
 * This is intended to be used during observation or scanning for devices to build up a list of devices found.
 * The key information about each device is its address and whether that address can be used to initiate a connection to the device
 * (devices that indicate they are connectable with a public address). 
 * 
 * Typical usage:
 * init_addr_list();
 * add_addr(addr); ... add_addr(addr);
 * print_addrs();
 * addr_enumeration_start();
 * end = !addr_enumeration_next(&some_addr, &int_connectable, &int_public_addr);
 * while (!end) { ... end = !addr_enumeration_next(&some_addr, &int_connectable, &int_public_addr); }
 * 
 * There is also a utility function to copy a device address from one place to another.
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef ADDRS_H
#define ADDRS_H

#include "HCI.h"

#define MAX_ADDRS 100

void copy_addr(tBDAddr from, tBDAddr * to);
void zero_addr(tBDAddr * addr);
bool addrs_match(tBDAddr addr1, tBDAddr addr2);
void print_addr(tBDAddr addr);

void init_addr_list();

void add_addr(tBDAddr newaddr, bool connectable, bool public_addr);

void print_addrs();

void addr_enumeration_start();
// connectable & public_addr values: 0 = false, 1 = true, -1 = both
bool addr_enumeration_next(tBDAddr * addr_ptr, int * connectable, int * public_addr); 

#endif
