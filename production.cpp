/*!
 * @file production.cpp
 * @brief Implementations of public and private production functions.
 * @details
 * The key implementation function is "run_production" which executes the action to be performed and checks each
 * expect, expect_ex, and global_expectation, running any associated action functions and checking the until condition.
 * 
 * Notes:
 * - The run_production is designed to be run repeatedly, doing the perform function only the first time, but everything
 * else repeatedly until the until condition is true.
 * - There are 3 possible return conditions (don't run again, run again but expectation was met, run again and expectation was not met)
 * - TODO: use something more flexible that fixed sized arrays.
 * - TODO: make use of the string version of the action name for debugging (e.g., the action fails).
 * 
 *  fine-print: copyright 2021 David Hamilton. This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).
 */

/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <STBLE.h>
#include <stdint.h>
#include <stddef.h>
#include "production.h"
#include "dbprint.h"

typedef struct rule_s {
  check_t check_type;
  uint16_t event_code;
  event_condition_ptr_t event_condition;
  event_action_ptr_t event_action;
  arg_t action_args;
} rule_t;

static action_ptr_t action = NULL;
static void * action_args = NULL;
void perform(action_ptr_t act, void * args) {
  action = act;
  action_args = args;
}
bool run_action_only_once() {
  bool ret;
  if (action) {
    ret = (*action)(action_args);
    if (ret) PRINTF("action %s returned true\n", get_action_name())
    else  PRINTF("action %s returned false\n", get_action_name())
    action = NULL;
    action_args = NULL;
  }
  else ret = true;
  return ret;
}

char action_name[MAX_ACTION_STRING_SIZE];
void set_action_name(char * act_name) {
  strncpy(action_name, act_name, MAX_ACTION_STRING_SIZE);
}
char * get_action_name() {
  return action_name;
}

static until_ptr_t until_condition = NULL;
void until(until_ptr_t until_function) {
  until_condition = until_function;
}
void until_clear() {
  until_condition = NULL;
}

static check_t until_check = no_check;
static uint16_t until_check_event = 0;
void until_event(check_t check_type, uint16_t event_code) {
  until_check = check_type;
  until_check_event = event_code;
}
void until_event_clear() {
  until_check = no_check;
  until_check_event = 0;
}


static unsigned long production_start;
static unsigned long timeout_milliseconds;
void set_timeout(unsigned long milliseconds) {
  timeout_milliseconds = milliseconds;
  start_timeout();
}
bool timeout(hci_event_pckt *event_pckt) {
  return ( (production_start + timeout_milliseconds) < millis());
}
void start_timeout() {
  production_start = millis();
}

static bool rule_matched;
bool met_expectations() {
  return rule_matched;
}

/////////////////////////////////////////////////////////////////////
//rules arrays: tbd: something more flexible than fixed length arrays
/////////////////////////////////////////////////////////////////////

rule_t rules[MAX_RULES];
rule_t exclusive_rules[MAX_RULES];
rule_t global_rules[MAX_RULES];
int num_rules = 0;
int num_exclusive_rules = 0;
int num_global_rules = 0;
int current_rule;
int current_global_rule;
int current_exclusive_rule;

void rules_clear() {
  num_rules = 0;
}

void exclusive_rules_clear() {
  num_exclusive_rules = 0;
}

void global_rules_clear() {
  num_global_rules = 0;
}

void rules_start() {
  current_rule = 0;
}

void exclusive_rules_start() {
  current_exclusive_rule = 0;
}

void global_rules_start() {
  current_global_rule = 0;
}

bool rules_done() {
  return (current_rule == num_rules);
}

bool exclusive_rules_done() {
  return (current_exclusive_rule == num_exclusive_rules);
}

bool global_rules_done() {
  return (current_global_rule == num_global_rules);
}

rule_t * new_rule() {
  if (current_rule == MAX_RULES) {
    DBMSG(DBL_ERRORS, "ERROR: exceeded max number of rules")
    return NULL;
  }
  return &(rules[num_rules++]);
}

rule_t * new_exclusive_rule() {
  if (current_exclusive_rule == MAX_RULES) {
    DBMSG(DBL_ERRORS, "ERROR: exceeded max number of exclusive rules")
    return NULL;
  }
  return &(exclusive_rules[num_exclusive_rules++]);
}

rule_t * new_global_rule() {
  if (current_global_rule == MAX_RULES) {
    DBMSG(DBL_ERRORS, "ERROR: exceeded max number of global rules")
    return NULL;
  }
  return &(global_rules[num_global_rules++]);
}

rule_t * next_rule() {
  if (rules_done()) return NULL;
  return &(rules[current_rule++]);
}

rule_t * next_exclusive_rule() {
  if (exclusive_rules_done()) return NULL;
  return &(exclusive_rules[current_exclusive_rule++]);
}

rule_t * next_global_rule() {
  if (global_rules_done()) return NULL;
  return &(global_rules[current_global_rule++]);
}

/////////////////////////////////////////////////////////////

void clear_all_expectations() {
  rules_clear();
  exclusive_rules_clear();
  global_rules_clear();
}

void clear_expectations() {
  rules_clear();
  rule_matched = false;
}

void clear_global_expectations() {
  global_rules_clear();
}

void clear_exclusive_expectations() {
  exclusive_rules_clear();
}

void set_expect(rule_t * r, check_t check_type, uint16_t event_code, event_action_ptr_t event_action, arg_t action_args) {
  if (!r) return;
  r->check_type = check_type;
  r->event_code = event_code;
  r->event_condition = NULL;
  r->event_action = event_action;
  r->action_args = action_args;
}

void set_expect_condition(rule_t * r, event_condition_t event_condition, event_action_ptr_t event_action, arg_t action_args) {
  if (!r) return;
  r->check_type = condition_check;
  r->event_code = 0;
  r->event_condition = event_condition;
  r->event_action = event_action;
  r->action_args = action_args;
}

void expect(check_t check_type, uint16_t event_code, event_action_ptr_t event_action, arg_t action_args) {
  rule_t * r = new_rule();
  set_expect(r, check_type, event_code, event_action, action_args);
}

void expect_condition(event_condition_t event_condition, event_action_ptr_t event_action, arg_t action_args) {
  rule_t * r = new_rule();
  set_expect_condition(r, event_condition, event_action, action_args);
}

void expect_ex(check_t check_type, uint16_t event_code, event_action_ptr_t event_action, arg_t action_args) {
  rule_t * r = new_exclusive_rule();
  set_expect(r, check_type, event_code, event_action, action_args);
}

void expect_ex_condition(event_condition_t event_condition, event_action_ptr_t event_action, arg_t action_args) {
  rule_t * r = new_exclusive_rule();
  set_expect_condition(r, event_condition, event_action, action_args);
}

void expect_globally(check_t check_type, uint16_t event_code, event_action_ptr_t event_action, arg_t action_args) {
  rule_t * r = new_global_rule();
  set_expect(r, check_type, event_code, event_action, action_args);
}

void expect_globally_condition(event_condition_t event_condition, event_action_ptr_t event_action, arg_t action_args) {
  rule_t * r = new_global_rule();
  set_expect_condition(r, event_condition, event_action, action_args);
}

/////////////////////////////////////////////////////////////

bool check4event(hci_event_pckt *event_pckt, check_t check_type, uint16_t event_code) {
  evt_le_meta_event *report_event_pckt;
  evt_blue_aci *evt_blue;
  evt_gap_procedure_complete * procedure_complete_pckt;
  evt_hal_initialized * reset_pckt;
  bool match = false;
  switch (check_type) {
    case event_check:
      if (event_pckt->evt == event_code) match = true;
      break;
    case le_meta_event_check:
      if (event_pckt->evt == EVT_LE_META_EVENT) {
        report_event_pckt = (evt_le_meta_event *) event_pckt->data;
        if (report_event_pckt->subevent == event_code) match = true;
      }
      break;
    case reset_reason:
      if (event_pckt->evt == EVT_VENDOR) {
        evt_blue = (evt_blue_aci *) (event_pckt->data);
        if (evt_blue->ecode == EVT_BLUE_HAL_INITIALIZED) {
          reset_pckt = (evt_hal_initialized *) (event_pckt->data);
          if (reset_pckt->reason_code == event_code) match = true;
        }
      }
      break;
    case ecode:
      evt_blue = (evt_blue_aci *) (event_pckt->data);
      if (evt_blue->ecode == event_code) match = true;
      break;
    case procedure_complete:
      if (event_pckt->evt == EVT_VENDOR) {
        evt_blue = (evt_blue_aci *) (event_pckt->data);
        if (evt_blue->ecode == EVT_BLUE_GAP_PROCEDURE_COMPLETE) {
          procedure_complete_pckt = (evt_gap_procedure_complete *) evt_blue->data;
          if (procedure_complete_pckt->procedure_code == event_code) match = true;
        }
      }
      break;
    default: match = false;
  };
  return match;
}

bool fire_rule(rule_t *r, hci_event_pckt *event_pckt) {
  bool do_action;
  if (r->check_type == condition_check) {
    if (r->event_condition && r->event_condition(event_pckt) ) do_action = true;
    else do_action = false;
  }
  else {
    do_action = check4event(event_pckt, r->check_type, r->event_code);
  }
  if (do_action) {
    if (r->event_action) r->event_action(event_pckt, r->action_args);
  }
  return do_action;
}

int run_production(void *pckt) {
  rule_t * r;
  bool did_rule = false;
  bool keep_running;
  hci_uart_pckt *hci_pckt;
  hci_event_pckt *event_pckt;
  hci_pckt = (hci_uart_pckt *) pckt;
  if (hci_pckt->type != HCI_EVENT_PKT) {
    DBMSG(DBL_HCI_EVENTS, "NON HCI_EVENT_PKT RECEIVED")
    keep_running = true;
    did_rule = false;
  }
  else {
    event_pckt = (hci_event_pckt*) (void*) hci_pckt->data;
    exclusive_rules_start();
    while (!exclusive_rules_done() && !did_rule) {
      r = next_exclusive_rule();
      if (fire_rule(r, event_pckt)) {
        did_rule = true;
        rule_matched = true;
      }
    }
    rules_start();
    //note that all non-exclusive rules that match will be done
    while (!rules_done()) {
      r = next_rule();
      if (fire_rule(r, event_pckt)) {
        did_rule = true;
        rule_matched = true;
      }
    }
    if (!did_rule) {
      global_rules_start();
      while (!global_rules_done() && !did_rule) {
        r = next_global_rule();
        if (fire_rule(r, event_pckt)) did_rule = true;
      }
    }
    /* keep running unless no until to check or either until function or until_event is true */
    keep_running = true;
    if (!until_condition && (until_check == no_check)) keep_running = false;
    if (until_condition && ((*until_condition)(event_pckt))) keep_running = false;
    if ((until_check != no_check) && check4event(event_pckt, until_check, until_check_event)) keep_running = false;
    if (!keep_running) {
      rules_clear();
      exclusive_rules_clear();
      until_clear();
      until_event_clear();
      return 0;
    }
  }
  if (did_rule) return 1;
  return -1;
}  
