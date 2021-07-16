/*!
 * @file production.h
 * @brief provides implementations of the keywords used to setup a product (expect, expect_globally, run_production, etc.)
 * @details
 * A production is a step to be run in a step function (see protocol.h for the framework in which productions are used).
 * More specifically, a production has a function that is run ("performed") which initiates some process that results in one or more events.
 * A production has a series of expectations on what events will occur and a pairing of a function (called an action) to call if/when that event occurs.
 * You can think of a pairing of an expectation and an action as being a rule, so a list of these pairings
 * can be thought of as a set of rules in a rule based production system like OPS5 or CLIPS, and this is why they are called productions.
 * A production can run one time (the default) or until a certain condition is true (by setting an until condition function).
 * The typical sequence of things to set up a production are:
 * expect_globally(type_of_expectation, event_expected, action_to_perform_when_event_occurs, option_function_to_check_expectation_instead_of_a_specific_event)
 * expect_globally(...)
 * PERFORM(some_function_to_start_the_process) 
 *    { or alternatively call perform() to set the function without retaining the string name of the function which is only used for debugging }
 * expect(type_of_expectation, event_expected, action_to_perform_when_event_occurs, option_function_to_check_expectation_instead_of_a_specific_event)
 * expect(...)
 * set_timeout(number_of_milliseconds)
 * until(your_own_function_to check_when_to_end_the_production_or_just_use_timeout)
 * 
 * You don't have to set up a until condition; if you don't then the production just runs once
 * Note that global expectations are not cleared from one production to the next but everything else is (expectations, actions, until).
 * Global expectations are really intended to check for error or unexpected events, and are only checked if a more specific expect does not match.
 * You can also call expect_ex instead of expect. The only difference is that all matching expectations are run but only the first expect_ex that matches.
 * If you really have one function that you want to perform when an event occurs, use expect_ex so it will stop searching after a match.
 * But if you have multiple things you want to do, then you can use multiple expects on the same event and all those responding actions will be done.
 * You can use the function expectations_met() to find out if any of your specific expect or expect_only were run, allowing you to check 
 * if your production succeeded. It returns false if none of them were run.  Note that global expectations will not be considered as part of expectations_met
 * since they are intended for error handling. 
 * 
 * As the above example shows, you can run a production for a given amount of time using set_timeout and until(timeout())
 * 
 * There are some convenience macros that can be used to add meaning to statements for more self-documenting code. 
 * These kind of have the effect of named arguments since C doesn't support named arguments.
 * These don't do anything; they are just for more readable statements:
 * 1. AND_DO(x) to indicate an action to be performed when an expectation is met.
 * 2. NO_ACTION to indicate that all you care about is that expectation is met and don't do anything specific as a result (other than continue with the protcol)
 * 3. WITH(x) to indicate an argument that is passed to an action. 
 * 4. NO_ARGS to indicate that no arguments need to be passed to an action.
 * 5. actions all have the same function signature (accepting a single void * argument and returning a bool) so there is an action_t typedef for this
 * 6. DUMMY_ARG to indicate that the action really doesn't have an argument, only a dummy arg to match the requried function signature
 * 7. SPECIFICALLY(x) is also only used when desired for extra readability. In the following first example it is used to indicate that not only
 * do you expect a meta_event but you specifically expect the connection complete meta event. In the second example, SPECIFICALLY is not used because without
 * this keywork, the statement can easily be read as expecting the ecode EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP.
 * 
 *     expect_ex(le_meta_event_check, SPECIFICALLY(EVT_LE_CONN_COMPLETE), AND_DO(get_connection_handle), WITH(&connection_handle));
 *     expect_ex(ecode, EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP, AND_DO(add_device_db_entry_from_event), WITH(&context));
 *     
 * If you don't like these keywords and think they add more clutter than useful documentation, then just don't use them.
 * 
 * The remaining functions and variables are intended to be used internally by the framework. See production.h and protocol.h/.cpp for more on how they are used.
 * 
 * Note: this and other modules in this project use a standard convention of macros being in all upper case
 * and never ending with a semicolon when using them. You can add a semicolon which should not affect compilation 
 * and you can always define another macro to provide a lower case name instead if you really don't like this convention
 * (or modify the original source to use lower case names).

 * 
 * TODO: provide a more flexible/efficient implemenation than fixed length arrays for the rules
 * 
 * TODO: allow rule priority to be set; as it is now, must just set them all in the order you want them to be tried 
 * 
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef PRODUCTION_H
#define PRODUCTION_H

#include <stdbool.h>
#include <STBLE.h>

// PUBLIC

// max for each of rules, exclusive rules, and global rules (so 60 total)
#define MAX_RULES 20

/* action function */
typedef bool (action_t)(void * args); 
typedef bool (*action_ptr_t)(void * args); 
#define PERFORM(act_name, args) { perform(act_name, args); set_action_name(#act_name); }
void perform(action_ptr_t action, void * args);
#define NO_ACTION NULL

/* expectations */
typedef enum {no_check, event_check, le_meta_event_check, ecode, reset_reason, procedure_complete, condition_check} check_t;
typedef bool (event_condition_t)(hci_event_pckt *event_pckt);
typedef bool (*event_condition_ptr_t)(hci_event_pckt *event_pckt);
typedef bool (event_action_t)(hci_event_pckt *event_pckt, void * args);
typedef bool (*event_action_ptr_t)(hci_event_pckt *event_pckt, void * args);
typedef void * arg_t;
#define NO_ACTION NULL
#define NO_ARGS NULL
#define DUMMY_ARG void * dummy /* use when no argument is needed except to match the function type required */

#define WITH(x) x
#define AND_DO(x) x
#define SPECIFICALLY(x) x

void expect(check_t check_type, uint16_t event_code, event_action_ptr_t event_action, arg_t action_args);
void expect_condition(event_condition_t event_function, event_action_ptr_t event_action, arg_t action_args);

void expect_ex(check_t check_type, uint16_t event_code, event_action_ptr_t event_action, arg_t action_args);
void expect_ex_condition(event_condition_t event_function, event_action_ptr_t event_action, arg_t action_args);

void expect_globally(check_t check_type, uint16_t event_code, event_action_ptr_t event_action, arg_t action_args);
void expect_globally_condition(event_condition_t event_function, event_action_ptr_t event_action, arg_t action_args);

bool met_expectations();

/* until condition */

typedef bool (until_t)(hci_event_pckt *event_pckt);
typedef bool (*until_ptr_t)(hci_event_pckt *event_pckt);
void until(until_ptr_t until_condition);
void until_event(check_t check_type, uint16_t event_code);

void set_timeout(unsigned long milliseconds);
bool timeout(hci_event_pckt *event_pckt);

// PRIVATE (only to be used by the protocol framework)

int run_production(void *pckt);

/* expectations */
void clear_expectations();
void clear_exclusive_expectations();
void clear_global_expectations();
void clear_all_expectations();

/* action */
bool run_action_only_once();
#define MAX_ACTION_STRING_SIZE 40
void set_action_name(char * act_name);
char * get_action_name();

/* until condition */
void until_clear();
void until_event_clear();

/* call to start the timeout in case it might be used */
void start_timeout();

#endif
