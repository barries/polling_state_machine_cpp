#pragma once

#include <stdbool.h>

// See the main state machine for example code

// Caller must define an enum PREFIX##_state_t before this, and place
// this in a struct so the other macros can access it.
#define PSM_DECLARE_STATE_MACHINE_FIELDS(state_t) \
    state_t state{};                              \
    state_t next_state{};                         \

// Action Block Keywords
// =====================
//
// For state machines where states don't need to share actions, all that's
// needed is a single state dispatcher, with each state's case containing that
// state's action blocks (some states may have no action blocks, some may have
// one or more):
//
//    PSM_DO_ACTIONS(traffic_light) {                    // Action Loop
//         switch (traffic_light.state) {                // State Dispatcher
//             case traffic_light_state_red:
//                 IF_ENTRY {                            // Action Block
//                      turn_on(&red_light);
//                 }
//                 IF_DO {                               // Action Block
//                      if (is_time_to_exit_state()) {
//                          traffic_light.next_state = traffic_light_state_green;
//                      }
//                 }
//                 IF_EXIT {                             // Action Block
//                      turn_off(&red_light);
//                 }
//                 break;
//            case traffic_light_state_green:
//                 ...
//        }
//    }
//
// Some rules:
//
// Each state's IF_ENTRY action(s) must precede its IF_DO action(s), which must
// precede its IF_EXIT action(s).
//
// To change to another state, set   my_sm.next_state   . To implement a transition
// out of and back into to the same state, running its IF_EXIT and IF_ENTRY blocks,
// change   my_sm.state   to some other state, ideally some equivalent of an unset
// value special state.
//
// If an IF_ENTRY action sets   my_sm.next_state   , the IF_DO actions will not
// run.
//
// To have multiple states share actions, define multiple dispatchers, one or
// more for the shared actions, and one or more for the state-specific actions.
//
// If a state has multiple IF_ENTRY action blocks and one of the first ones alters
// my_sm.next_state  , the ones will run; the state is fully entered.
//
// If a state has mulitple IF_DO actions and an early on alters   my_sm.next_state,
// the remaining ones *won't* run, because that state is now exiting.
//
// If a state's IF_EXIT block changes    my_sm.next_state   back to the current state
// later IF_EXIT blocks for that state will not run, and the state will remain in
// that state--it cancels the transition (it won't be re-entered).
//
#define IF_ENTRY if (*psm_state !=  psm_prev_state)
#define IF_DO    if (*psm_state == *psm_next_state)
#define IF_EXIT  if (*psm_state != *psm_next_state)

// Wrap all of the state machine's actions logic in a loop using this macro as though it were
// a for loop.
#define PSM_DO_ACTIONS(instance)                                           \
    for (                                                                  \
        typeof((instance).state)  psm_prev_state =  (instance).state,      \
                                 *psm_state      = &(instance).state,      \
                                 *psm_next_state = &(instance).next_state; \
        psm_state != NULL;                                                 \
        [&] () {                                                           \
            psm_prev_state   = (instance).state;                           \
            (instance).state = (instance).next_state;                      \
            if (psm_prev_state == (instance).next_state) {                 \
                psm_state = NULL; /* No change, exit the loop */           \
            }                                                              \
        }()                                                                \
    )                                                                      \

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Robert Barrie Slaymaker, Jr.
//
// License: Creative Commons Attribution 4.0 Internaltional License (CC BY 4.0).
//
// https://creativecommons.org/licenses/by/4.0/
//
// Attribution — You must give appropriate credit, provide a link to the
// license, and indicate if changes were made. You may do so in any reasonable
// manner, but not in any way that suggests the licensor endorses you or your
// use.
//
// No additional restrictions — You may not apply legal terms or technological
// measures that legally restrict others from doing anything the license
// permits.
//
// Full license is hereby granted to FreshHealth and any company that acquires
// license to this source code from them.
////////////////////////////////////////////////////////////////////////////////

