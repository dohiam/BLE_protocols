/*!
 * @file procedures.h
 * @brief provides procedures that can be used as things to perform or actions to do when an expectation is met
 * @details
 * In addition to wrapping various BlueRNG procedure functions with a signature that can be used by the framework, these 
 * implementations also include error checking, optional debug output, and provide reasonable default values (see procedures.cpp for more)
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef OBSERVER_H
#define OBSERVER_H

#include <STBLE.h>
#include "production.h"
#include "get_data.h"

bool start_observation(DUMMY_ARG);

bool start_directed_scan(DUMMY_ARG);

action_t start_connection; /* pass pointer to address to connect to as void * */
action_t terminate_connection; /* argument should be (arg_t) &connection_handle */
action_t terminate_gap_procedure; 
/* argument should be (arg_t) &procedure_code (uint8_t) one of: 
 *  GAP_LIMITED_DISCOVERY_PROC,                  
 *  GAP_GENERAL_DISCOVERY_PROC,                  
 *  GAP_NAME_DISCOVERY_PROC,                     
 *  GAP_AUTO_CONNECTION_ESTABLISHMENT_PROC,      
 *  GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC,   
 *  GAP_SELECTIVE_CONNECTION_ESTABLISHMENT_PROC, 
 *  GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC,    
 *  GAP_OBSERVATION_PROC_IDB05A1
 */

event_action_t handle_connection_update;

bool discover_primary_services(arg_t connection_handle);

bool discover_included_services(arg_t attribute_info);

bool discover_characteristcs(arg_t attribute_info);

#endif
