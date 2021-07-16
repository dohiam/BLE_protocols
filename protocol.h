/*!
 * @file protocol.h
 * @brief Provides a framework for running protocols on systems without multi-tasking.
 * @details
 * In this context, a protocol is a set of rules for back and forth communication between two entities.
 * The purpoe of these macros is to enable "step functions", which are functions that are called repeatedly 
 * doing one small piece of work each time they are called. but each time they are called.
 * Using this protocol framework allows step functions to appear as though they are purely
 * sequential threads running in a multi-threaded system that only yield at the end of each step.
 * In other words, to make functions in a non-threaded system appear to be threads.
 *  Example:
 *    PROTOCOL (protocol_name)
 *    ... {declare local variables, do initialization stuff}
 *    BEGIN_PROTOCOL 
 *      perform(..)
 *        expect(..)
 *        expect(..)
 *      RUN_PRODUCTION  {yields to allow that step to be executed by other "virtual" threads in the system}
 *      if PROTOCOL_IS_WORKING
 *        perform(..)
 *          expect(..)
 *          expect(..)
 *      RUN_PRODUCTION_AND_REPEAT_IF(some_condition)
 *    ... {stuff to run on the last step}
 *    END_PROTOCOL {ends this "virtual thread"}
 *    
 *  This will define a function using protocol_name. If you need a forward declaration for this function,
 *  you can use the typedef protocol_t to declare it.
 *  
 *  Note: this and other modules in this project use a standard convention of macros being in all upper case
 *  and never ending with a semicolon when using them. You can add a semicolon which should not affect compilation 
 *  and you can always define another macro to provide a lower case name instead if you really don't like this convention
 *  (or modify the original source to use lower case names).
 *  
 *  There is a bool variable called protocol_success but it is recommended using PROTOCOL_IS_WORKING in
 *  case the implementation of protocol status changes in the future. You can use ABORT_PROTOCOL to indicate
 *  the protocol has failed. The framework will also automatically abort protocols if it detects an error.
 *  
 *  There is also a bool variable called ret that is used internally to get success or failure of
 *  functions called inside the framework; you can also use this variable for temporary status of
 *  functions you call too.
 *  
 *  See production.h for things that can be used to set up productions.
 *  
 *  Framework implementation notes:
 *  - much of the implemenation is inside the macros in this header file.
 *  - the mechanism to enable the step function to pick up at the last step is using two counters: 
 *    one counter (state) is static and keeps track of the next step to be run and the other
 *    counter (state_compare) increments each time it is compared to the step to be run.
 *    So when the function first starts, both are zero and the first comparison succeeds. Then
 *    the step counter (state) is incremented so that the next time the function is called, the
 *    second comparison succeeds, and so on.
 *    PROTOCOL does the function and local variables declarations
 *  - BEGIN_PROTOCOL sets up the first comparison step
 *  - RUN_PRODUCTION ends the comparison and sets up the next comparison. 
 *  - RUN_PRODUCTION_AND_REPEAT_IF(some_condition) works like an until statement for the production. 
 *  - END_PROTOCOL ends the last comparison and the protocol function
 *  
 *  See protocol.cpp for the implementations of the functions to set, get, and run the current protocol.
 *  run_current_protocol is what needs to be called by the HCI event callback.
 *  
 *  There is a similar and simpler framework to execute a generic step function that isn't a protocol.
 *  Hopefully the following example is sufficient to see how to use this framework:
 *  STEP_FUNCTION(name)
 *    SKIP_STEPS_IF(some_condition) // just returns without running any step
 *    FIRST_STEP
 *       ... your logic here
 *     NEXT_STEP
 *       .... your logic here
 *       REPEAT_STEP_WHILE(some_condition)
 *     LAST_STEP
 *       ... your logic here
 *  END_STEP_FUNCTION
 *  
 *  The above will be run one step each time it is called, except for:
 *    The function returns immediately without trying to run any steps if some_condition on SKIP_STEPS_IF is true.
 *    The second step will be run each time the function is called while some_condition on REPEAT_STEP_WHILE is true.
 *    Any return in your logic will have the effect of doing that step again when the function is called again, so
 *    there is a RETURN_STEP macro that has the effect of returning but picking up at the next step next time the function is called.
 *    
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef bool (*protocol_ptr_t)();
typedef bool (protocol_t)();

void run_current_protocol(void *pckt);
void set_current_protocol(protocol_ptr_t protocol);
protocol_ptr_t get_current_protocol();
void clear_current_protocol();
void wait_for_protocol_finish();
bool protocol_running();

//There is a string version of the protocol name used in debugging statements, limited to the following size. 
#define MAX_PROTOCOL_STRING_SIZE 40
void set_protocol_name(char * proto_name);
char * get_protocol_name();

/*
 * Here are the macros which provide a domain specific language for protocol functions.
 * 
 */

#define PROTOCOL(protocol_name)     \
bool protocol_name() {              \
  uint16_t state_compare = 0;       \
  static uint16_t state = 0;        \
  bool protocol_success = true;     \
  bool ret;

#define BEGIN_PROTOCOL(protocol_name)    \
  if (state == state_compare++) {        \
    set_current_protocol(protocol_name); \
    set_protocol_name(#protocol_name);

#define RUN_PRODUCTION                   \
    ret = run_action_only_once();        \
    if (!ret) {                          \
      PRINTF("action %s failed, aborting protocol %s \n", get_action_name(), get_protocol_name()); \
      clear_current_protocol();         \
    }                                   \
    else state++;                       \
    return protocol_success;            \       
  }                                     \
  if (state == state_compare++) {  

#define RUN_PRODUCTION_AND_REPEAT_IF(cond) \
    if (!protocol_success) return false;   \
    ret = run_action_only_once();          \
    if (!ret) {                            \
      PRINTF("action %s failed, aborting protocol %s \n", get_action_name(), get_protocol_name()); \
      clear_current_protocol();            \
    }                                      \
    else {                                 \ 
      if (!(cond)) state++;                \
    }                                      \
    return true;                           \       
  }                                        \
  if (state == state_compare++) {  

#define ABORT_PROTOCOL                     \
  protocol_success = false;                \
  return false;

#define PROTOCOL_IS_WORKING         \
  protocol_success = true;

#define IS_PROTOCOL_WORKING         \
   protocol_success

#define END_PROTOCOL                \
    state = 0;                      \
    clear_current_protocol();       \
    return protocol_success;        \
  }                                 \
}

/*
 * Here are the macros which define a domain specific language for STEP functions
 */

#define STEP_FUNCTION(sf_name)      \
void sf_name() {                    \
  uint16_t state_compare = 0;       \
  static uint16_t state = 0;

#define FIRST_STEP                  \
  if (state == state_compare++) {

#define NEXT_STEP                   \
    state++;                        \
    return;                         \       
  }                                 \
  if (state == state_compare++) {  

#define REPEAT_STEP_WHILE(condition_arg)   \
    if (condition_arg) return; 

#define RETURN_STEP                 \
    { state++; return; } 

#define SKIP_STEPS_IF(condition_arg)   \
    if (condition_arg) return; 

#define END_STEP_FUNCTION           \
    state++;                        \
    return;                         \
  }                                 \
}

#endif
