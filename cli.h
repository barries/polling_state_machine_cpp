#pragma once

#include "attributes.h"
#include "log.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// Internals

void cli_macro_impl_print(      const char *string_0, ...); // private
void cli_macro_impl_print_error(const char *string_0, ...); // private

////////////////////////////////////////////////////////////////////////////////
// API

void cli_init(void);
void cli_start(void);
void cli_poll(void);
void cli_print_log(void);

void cli_prepare_for_repeat(void);

nodiscard bool cli_is_in_shell_mode(void);

#define cli_print(...)       cli_macro_impl_print(      __VA_ARGS__, NULL)
#define cli_print_error(...) cli_macro_impl_print_error(__VA_ARGS__, NULL)

void cli_print_byte_from_handle(uint8_t b);

void cli_print_log_event(log_event_ptr_t event);
void cli_print_log_prefix(void);

void cli_logf(        const char *format_string, ...) __attribute__ ((format (printf, 1, 2)));
void cli_printf(      const char *format_string, ...) __attribute__ ((format (printf, 1, 2)));
void cli_printf_error(const char *format_string, ...) __attribute__ ((format (printf, 1, 2))); // TODO: rename to indicate EOL is also printed

