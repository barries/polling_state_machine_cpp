#include "polling_state_machine.h"

#include "X_macro_helpers.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <thread>

// This file is a mock of a typical bare-metal MCU main.c.

////////////////////////////////////////////////////////////////////////////////
// Mocks

// Timekeeping typical of a tiny bare metal MCU

typedef size_t ms_t;

static ms_t now_ms;

[[nodiscard]] static size_t elapsed_ms(ms_t ms) {
    return now_ms > ms
        ? (size_t)(now_ms - ms)
        : 0u;
}

// Logging

static void emit_log_prefix() {
    std::cout << std::right; // Restore default
    std::cout << std::setw(5) << now_ms << " ";
}

#define LOG(symbol) [&] () {                       \
    emit_log_prefix();                             \
    std::cout << #symbol ": " << (symbol) << "\n"; \
}()

// Output controller for the actual lamps

static void set_light(const char *name, bool on) {
    static char prev_name[10];
    static bool prev_on = false;
    if (strcmp(name, prev_name) != 0 || on != prev_on) {
        emit_log_prefix();

        std::cout
            << "set_light(): "
            << name
            << " "
            << (on ? "on" : "off" )
            << "\n";

        strncpy(prev_name, name, sizeof(prev_name) - 1);
        prev_on = on;
    }
}

// Unusual Conditions

static bool some_hw_error_exists;

static void simulate_hw_error(bool some_hw_error_exists_) {
    if (some_hw_error_exists != some_hw_error_exists_) {
        some_hw_error_exists = some_hw_error_exists_;
        LOG(some_hw_error_exists);
    }
}

static bool emergency_vehicle_detected;

static void simulate_emergency_vehicle_detected(bool emergency_vehicle_detected_) {
    if (emergency_vehicle_detected != emergency_vehicle_detected_) {
        emergency_vehicle_detected = emergency_vehicle_detected_;
        LOG(emergency_vehicle_detected);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Stoplight State Machine

#define FOREACH_STOPLIGHT_STATE_MACHINE_STATE(X) \
    X(Red)                                       \
    X(Yellow)                                    \
    X(Green)                                     \
    X(Errored)                                   \
    X(Faulted)                                   \

#define LOG_STATE(state_symbol) [&] () {                                                            \
    emit_log_prefix();                                                                              \
    std::cout << #state_symbol ": " << stoplight_sm_t::state_names[(size_t)(state_symbol)] << "\n"; \
}()

class stoplight_sm_t {
public:

    void handle_error_event() {
        emit_log_prefix();
        std::cout << "handle_error_event()\n";
        set_next_state(state_t::Errored);
    }

    void handle_error_cleared_event() {
        emit_log_prefix();
        std::cout << "handle_error_cleared_event()\n";
        set_next_state(state_t::Red);
    }

    void poll() { // Call 1/ms
        PSM_DO_ACTIONS(*this) {

            // Logic common to all states.
            IF_ENTRY {
                LOG_STATE(state);
                state_entered_ms = now_ms;
            }
            IF_DO {
                     if (some_hw_error_exists                                  ) { set_next_state(state_t::Faulted); }
                else if (state < state_t::Faulted && some_hw_error_exists      ) { set_next_state(state_t::Errored); }
                else if (state < state_t::Errored && emergency_vehicle_detected) { set_next_state(state_t::Red    ); }
            }

            switch (state) {
                case state_t::unset:
                    // ...initialize things here...
                    set_next_state(state_t::Red);
                    break;

                case state_t::Red:
                    IF_ENTRY {
                         set_light("Red", true);
                    }
                    IF_DO {
                        if (elapsed_ms() > 5000) {
                            set_next_state(state_t::Green);
                        }
                    }
                    IF_EXIT {
                         set_light("Red", false);
                    }
                    break;

                case state_t::Yellow:
                    IF_ENTRY {
                         set_light("Yellow", true);
                    }
                    IF_DO {
                        if (elapsed_ms() > 1000) {
                            set_next_state(state_t::Red);
                        }
                    }
                    IF_EXIT {
                         set_light("Yellow", false);
                    }
                    break;

                case state_t::Green:
                    IF_ENTRY {
                         set_light("Green", true);
                    }
                    IF_DO {
                        if (elapsed_ms() > 5000) {
                            set_next_state(state_t::Yellow);
                        }
                    }
                    IF_EXIT {
                         set_light("Green", false);
                    }
                    break;

                case state_t::Errored:
                    IF_ENTRY {
                         set_light("Red", true);
                    }
                    IF_DO {
                        set_light("Red", (elapsed_ms() % 2000) >= 1000);
                    }
                    IF_EXIT {
                        set_light("Red", false);
                    }
                    break;

                case state_t::Faulted:
                    IF_ENTRY {
                         set_light("Red", true);
                    }
                    IF_DO {
                        set_light("Red", (elapsed_ms() % 2000) >= 1000);
                    }
                    IF_EXIT {
                        reject_transition(); // Don't allow exit from state, require power cycle
                    }
                    break;
            }
        }
    }

private:

    enum class state_t {
        unset,
        FOREACH_STOPLIGHT_STATE_MACHINE_STATE(DECLARE_NAME)
    };

    static const char *const state_names[];

    [[nodiscard]] size_t elapsed_ms() {
        return ::elapsed_ms(state_entered_ms);
    }

    void set_next_state(state_t requested_next_state) {
        if (requested_next_state != next_state) {
            LOG_STATE(requested_next_state);
        }

        if (state != state_t::Errored && next_state == state_t::Errored) {
            emit_log_prefix();
            std::cout << "set_next_state(): rejected transition while heading to Errored state\n";
            return; // ignore, next_state_ and go to error state even if a later call tries to go to a normal state.
        }

        next_state = requested_next_state;
    }

    void reject_transition() { // Can be called in IF_DO or IF_EXIT (not IF_ENTRY; by then it's too late)
        if (next_state != state) {
            auto& rejected_transition = next_state;
            LOG_STATE(rejected_transition);
            next_state = state;
        }
    }

    // Data members

    PSM_DECLARE_STATE_MACHINE_FIELDS(state_t)

    size_t state_entered_ms;
};

const char *const stoplight_sm_t::state_names[] = {
    "unset",
    FOREACH_STOPLIGHT_STATE_MACHINE_STATE(DECLARE_STRING)
};

////////////////////////////////////////////////////////////////////////////////
// Mock a typical embedded system main loop or timer tick IRQ handler

int main() {
    stoplight_sm_t sm;

    while (true) {
        switch (elapsed_ms(0)) { // Deliver some events to sm
            case 80000: simulate_emergency_vehicle_detected(true);  break;
            case 10000: sm.handle_error_event();                    break;
            case 11000: simulate_emergency_vehicle_detected(false); break;
            case 15000: sm.handle_error_cleared_event();            break;
            case 16000: simulate_hw_error(true);                    break;
            case 17000: sm.handle_error_cleared_event();            break;
            case 18000: simulate_hw_error(false);                   break;
            case 19000: sm.handle_error_cleared_event();            break;
            case 30000: return 0;                                   break;
        }

        sm.poll();

        // Uncomment for verisimilitue: std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ++now_ms;
    }

}
