#include "cli.h"

#include "NUM_ARRAY_ELTS.h"
#include "app.h"
#include "attributes.h"
#include "base_errors.h"
#include "base_psp.h"
#include "base_usp.h"
#include "bldc.h"
#include "log.h"
#include "microrl.h"
#include "shell.h"
#include "pac5xxx_critical_section.h"
#include "printf.h"
#include "psp.h"
#include "ring_buffer.h"
#include "tick_timer.h"

#include <pac5xxx.h>
#include <pac5xxx_driver_uart.h>
#include <stdarg.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Internals

#define cli_max_handle_response_time_ms 20  // 20ms: more than enough for handle to send a response

typedef enum {
    cli_mode_idle,      // Nothing is printing, cursor is on new line (i.e. no shell prompt displayed
    cli_mode_shell,     // Shell prompt is being edited
    cli_mode_printing,  // Program is printing information
    cli_mode_base_psp,  // Forwading data to/from base's PSP server
    cli_mode_handle_usp // Forwarding data to/from handle (via umbilical UART)
} cli_mode_t;

// Buffer incoming characters so that they can be handled in the main thread
// and handlers won't run on the interrupt thread.
static ring_buffer_t cli_rx_buf;
static uint8_t       cli_rx_buf_data[128]; // 128: power of 2, a reasonable number of pasted chars, 5 to 10 commands' worth.

static ring_buffer_t cli_tx_buf;
static uint8_t       cli_tx_buf_data[128]; // 128: power of 2, more than a line's worth.

static ms_t       cli_activity_timestamp_ms;
static cli_mode_t cli_mode;
static bool       cli_shell_has_pending_changes;
static ms_t       cli_pending_changes_timestamp_ms;

RAMFUNC static void xmit_next_char(void) {
    uint8_t b;
    if (rb_pop(&cli_tx_buf, &b)) {
        PAC55XX_UARTC->THR.THR = b;
        cli_activity_timestamp_ms = tt_now_ms();
    }
}

RAMFUNC static void cli_transmit_byte(uint8_t b) {
    if (!PAC55XX_UARTC->IER.THRIE) {
        while (!PAC55XX_UARTC->LSR.THRE) {
        }
        PAC55XX_UARTC->THR.THR = b;
        cli_activity_timestamp_ms = tt_now_ms();
    }
    else {
        bool cs_state = enter_critical_section();

        bool was_empty = rb_is_empty(&cli_tx_buf);

        while (!rb_push(&cli_tx_buf, b)) {
            exit_critical_section(cs_state);
            cs_state = enter_critical_section();
        }

        if (was_empty && PAC55XX_UARTC->LSR.THRE) {
            xmit_next_char();
        } else {
            cli_activity_timestamp_ms = tt_now_ms();
        }

        exit_critical_section(cs_state);
    }
}

RAMFUNC static void cli_transmit_str(const char *s) {
    while (*s != '\0') {
        cli_transmit_byte((uint8_t)*s);
        ++s;
    }
}

static void cli_set_mode(cli_mode_t new_mode) {
    if (cli_mode != new_mode) {
        switch (cli_mode) {
            case cli_mode_shell:
                if (new_mode != cli_mode_idle) {
                    cli_transmit_str("\r");
                }
                break;

            case cli_mode_printing:
                if (new_mode == cli_mode_shell) {
                    cli_transmit_str("\r\n");
                }
                break;

            default:
                break;
        }
        cli_mode = new_mode;

        switch (cli_mode) {
            case cli_mode_shell:
                shell_display_cmd_line();
                cli_shell_has_pending_changes = false;
                break;

            default:
                break;
        }
    }
}

RAMFUNC void _putchar(char c) { // Also called by printf.c
    if (cli_mode == cli_mode_base_psp || cli_mode == cli_mode_handle_usp) {
        return;
    }

    cli_set_mode(cli_mode_printing);

    if (c == '\n') {
        cli_transmit_byte('\r');
    }
    cli_transmit_byte((uint8_t)c);

    if (c == '\n') {
        cli_set_mode(cli_mode_idle);
    }
}

static void cli_print_buf_n(size_t num_bytes, const void *buf) {
    const char *p           = (const char *)buf;
    const char *const p_end = p + num_bytes;
    while (p != p_end) {
        _putchar(*p++);
    }
}

static void cli_print_string(const char *s) {
    while (*s) {
        _putchar(*s++);
    }
}

static void cli_print_shell_cmd_line_string(const char *s) {
    if (!(cli_mode == cli_mode_idle || cli_mode == cli_mode_shell)) {
        cli_shell_has_pending_changes    = true;
        cli_pending_changes_timestamp_ms = tt_now_ms();
        return;
    }

    bool is_idle = (cli_mode == cli_mode_idle);

    while (*s) {
        is_idle = (*s == '\n');
        if (is_idle) {
            cli_transmit_byte('\r');
        }
        cli_transmit_byte((uint8_t)*s++);
    }

    if (is_idle) {
        cli_set_mode(cli_mode_idle);
    } else {
        cli_mode = cli_mode_shell; // Set directly to avoid (re)printing the prompt
    }
}

static void cli_transmit_psp_byte(uint8_t b) {
    cli_set_mode(cli_mode_base_psp);
    cli_transmit_byte(b);
}

static void cli_macro_impl_vprint(va_list strings) {
    while (true) {
        const char *string_n = va_arg(strings, const char *);
        if (string_n == NULL) {
            break;
        }
        cli_print_string(string_n);
    }
}

static void cli_print_error_prefix(void) {
    const char *const error__ = "error: ";

    cli_print_log_prefix();
    cli_print_string(error__);
}

void cli_macro_impl_print(const char *string_0, ...) {
    cli_print_string(string_0);

    va_list strings_1_to_N;
    va_start(strings_1_to_N, string_0);
    cli_macro_impl_vprint(strings_1_to_N);
    va_end(strings_1_to_N);
}

void cli_macro_impl_print_error(const char *string_0, ...) {
    cli_print_error_prefix();
    cli_print_string(string_0);

    va_list strings_1_to_N;
    va_start(strings_1_to_N, string_0);
    cli_macro_impl_vprint(strings_1_to_N);
    va_end(strings_1_to_N);
    _putchar('\n');
}

////////////////////////////////////////////////////////////////////////////////
// API

void cli_init(void) {
    cli_tx_buf = RB_CREATE(cli_tx_buf_data);
    cli_rx_buf = RB_CREATE(cli_rx_buf_data);

    PAC55XX_SCC->CCSCTL.USCMODE = USART_MODE_UART;

    PAC55XX_UARTC->LCR.WLS  = UARTLCR_WL_BPC_8;
    PAC55XX_UARTC->LCR.SBS  = UART_STOP_BITS_1;
    PAC55XX_UARTC->LCR.PEN  = UART_PEN_ENABLE;
    PAC55XX_UARTC->LCR.PSEL = UART_PARITY_EVEN;
    PAC55XX_UARTC->LCR.BCON = UART_BRKCTL_DISABLE;

    PAC55XX_UARTC->DLR.w = 27;// 115kbps = PCLK / (16 * UARTBDLR) (actually 115740...)

    PAC55XX_UARTC->FCR.FIFOEN    = 1;             // Enable FIFO

    pac5xxx_uart_io_config2(PAC55XX_UARTC);

    PAC55XX_SCC->PEPUEN.P3 = 1; // enable pull-up to address noise causing RX issues

    NVIC_ClearPendingIRQ(USARTC_IRQn);
    NVIC_EnableIRQ(USARTC_IRQn);

    _putchar('\n');
    log_msg("========================================");

    bpsp_init(cli_transmit_psp_byte);
}

bool cli_is_in_shell_mode(void) {
    return cli_mode == cli_mode_shell;
}

void cli_poll(void) {
    shell_poll();

    if (
           (cli_mode == cli_mode_printing && cli_shell_has_pending_changes && tt_elapsed_since_ms(cli_pending_changes_timestamp_ms) >= 500)  // Program is printing, but butt in anyway in case it's not stopping...
        || ((cli_mode == cli_mode_idle || cli_mode == cli_mode_printing)   && tt_elapsed_since_ms(cli_activity_timestamp_ms       ) >=  50)  // Program has stopped printing
    ) {
        cli_set_mode(cli_mode_shell);
    }

    uint8_t b;
    while (rb_pop(&cli_rx_buf, &b)) {
        if (cli_mode == cli_mode_idle || cli_mode == cli_mode_shell || cli_mode == cli_mode_printing) {
            if (psp_get_base_msg_type_lookup_offset(b) >= 0) {
                cli_set_mode(cli_mode_base_psp);
            }
            else if (psp_get_handle_msg_type_lookup_offset(b) >= 0) {
                if (app_is_in_normal_operation()) {
                    app_enter_Test_Mode(); // Silence the polling to prevent contention.
                }
                cli_set_mode(cli_mode_handle_usp);
            }
        } else if (
               (cli_mode == cli_mode_base_psp   && b < 127 && (!bpsp_is_receiving_msg() && tt_elapsed_since_ms(cli_activity_timestamp_ms) >= psp_min_idle_time_ms           ))
            || (cli_mode == cli_mode_handle_usp && b < 127 && (                            tt_elapsed_since_ms(cli_activity_timestamp_ms) >= cli_max_handle_response_time_ms))
        ) {
            cli_set_mode(cli_mode_shell);
        }

        cli_activity_timestamp_ms = tt_now_ms();
        switch (cli_mode) {
            case cli_mode_base_psp:
                bpsp_process_received_byte(b);
                break;

            case cli_mode_handle_usp:
                busp_forward_byte_to_handle(b);
                break;

            default:
                shell_process_input_char((char)b);
                break;
        }
    }
}

void cli_prepare_for_repeat(void) {
    if (cli_mode == cli_mode_shell) {
        cli_mode = cli_mode_printing; // Set directly to avoid printing a newline.
        cli_print_string("\r");
    }
}

void cli_logf(const char *format_string, ...) {
    cli_print_log_prefix();

    va_list args;
    va_start(args, format_string);
    vprintf(format_string, args);
    va_end(args);
    _putchar('\n');
}

void cli_print_log_event(log_event_ptr_t event) {
    cli_printf("%10lu ", event->timestamp_ms);

    const uint8_t *p = event->payload_bytes;

    size_t num_name_bytes = 9999;

    if (event->type == log_event_type_unset_error) {
        cli_print_string("unset");
    } else if (event->type == log_event_type_discarded_events) {
        cli_printf("**** discarded %lu log events due to log_buffer[] overflow ****", *(const uint32_t *)(const void *)p);
    } else {
        // print msg, error, or name
        size_t num_bytes = *p++;
        if (num_bytes == 0) {
            cli_print_string("\"\"");
        } else {
            cli_print_buf_n(num_bytes, p);
            num_name_bytes = num_bytes;
        }
        p += num_bytes;
    }

    if (
           event->type != log_event_type_discarded_events
        && event->type != log_event_type_error
        && event->type != log_event_type_msg
    ) {
        _putchar(':');
        _putchar(' ');

        for (size_t i = num_name_bytes; i < 24; ++i) { // 24: wider than most names
            _putchar(' ');
        }

        switch (event->type) {
            case log__end_of_log_sentinel__event_type:
            case log_event_type_discarded_events:
            case log_event_type_error:
            case log_event_type_msg:
                // Noop
                break;

            case log_event_type_string:
            case log_event_type_unset_error: {
                size_t num_bytes = *p++;
                if (num_bytes == 0) {
                    cli_print_string("\"\"");
                } else {
                    cli_print_buf_n(num_bytes, p);
                }
                break;
            }

            case log_event_type_bool_false: cli_print_string("false"); break;
            case log_event_type_bool_true:  cli_print_string("true" ); break;
            case log_event_type_bool_off:   cli_print_string("off"  ); break;
            case log_event_type_bool_on:    cli_print_string("on"   ); break;
            case log_event_type_bool_low:   cli_print_string("low"  ); break;
            case log_event_type_bool_high:  cli_print_string("high" ); break;

            case log_event_type_uint:  cli_printf("%6lu",           *(const uint32_t *)(const void *)p); break;
            case log_event_type_hex:   cli_printf("0x%08lx",        *(const uint32_t *)(const void *)p); break;
            case log_event_type_float: cli_printf("%10.3f", (double)*(const float    *)(const void *)p); break;
        }
    }

    _putchar('\n');
}

void cli_print_log(void) {
    static const char cut_here_line[] = "----8<----8<----8<----8<----8<----8<----";

    cli_mode_t saved_mode = cli_mode; // Save, so we can return to PSP mode if we happen to be init.

    cli_print(cut_here_line, " start of log ", cut_here_line, "\n");

    bool cs_state = enter_critical_section();
    log_lock();
    for (log_event_ptr_t event = log_read(NULL); event->type != log__end_of_log_sentinel__event_type; event = log_read(event)) {
        exit_critical_section(cs_state);
        cli_print_log_event(event);
        cs_state = enter_critical_section();
    }
    exit_critical_section(cs_state);
    log_unlock();
    cli_print(cut_here_line, " end_of_log ", cut_here_line, "\n");

    cli_set_mode(saved_mode);
}

void cli_print_log_prefix(void) {
    cli_printf("%10lu ", tt_now_ms());
}

void cli_printf(const char *format_string, ...) {
    va_list args;
    va_start(args, format_string);
    vprintf(format_string, args);
    va_end(args);
}

void cli_printf_error(const char *format_string, ...) {
    cli_print_error_prefix();

    va_list args;
    va_start(args, format_string);
    vprintf(format_string, args);
    va_end(args);
    _putchar('\n');
}

void cli_print_byte_from_handle(uint8_t b) {
    if (cli_mode != cli_mode_handle_usp) {
        cli_set_mode(cli_mode_printing); // Assume it's handle stdio output
    }
    cli_transmit_byte(b);
    if (b == '\n') {
        cli_set_mode(cli_mode_idle);
    }
}

void cli_start(void) {
    shell_init(cli_print_shell_cmd_line_string);

    // Enable Interrupts
    PAC55XX_UARTC->IER.RLSIE = 1; // Receive Line Statue IE
    PAC55XX_UARTC->IER.RBRIE = 1; // Receive Buffer Register IE
    PAC55XX_UARTC->IER.THRIE = 1; // Transmit Hold Register (empty) IE
}

////////////////////////////////////////////////////////////////////////////////
// IRQ handler

void USARTC_IRQHandler(void) {
    switch (PAC55XX_UARTC->IIR.INTID) {
        case UARTIIR_INTID_TX_HOLD_EMPTY:
            xmit_next_char();
            break;

        case UARTIIR_INTID_RX_LINE_STATUS: {
            uint32_t lsr = PAC55XX_UARTC->LSR.w;
            if (lsr & 0x1e) { // 0x1e: Break, Framing, Parity or Overrun
                if      (lsr & 0b00010000) { be_set_and_log(BaseShellSerialBreakError  ); }
                else if (lsr & 0b00001000) { be_set_and_log(BaseShellSerialFramingError); }
                if      (lsr & 0b00000100) { be_set_and_log(BaseShellSerialParityError ); }
                if      (lsr & 0b00000010) { be_set_and_log(BaseShellSerialOverrunError); }
                PAC55XX_UARTC->FCR.RXFIFORST = 1;
                PAC55XX_UARTC->FCR.TXFIFORST = 1;

                bpsp_clear_rx_and_rx_msg();
                shell_clear_cmd_line();
                cli_set_mode(cli_mode_idle);
            }
            break;
        }

        case UARTIIR_INTID_FIFO_TIMEOUT:
            PAC55XX_UARTC->FCR.RXFIFORST = 1;
            break;

        case UARTIIR_INTID_RX_DATA_AVAIL:
            while (PAC55XX_UARTC->LSR.RDR) {
                uint8_t b = PAC55XX_UARTC->RBR.RBR;
                bool dummy = rb_push(&cli_rx_buf, b);
                (void)dummy;
            }
            break;

        default:
            break;
    }

    NVIC_ClearPendingIRQ(USARTC_IRQn);
}
