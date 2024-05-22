/*
Author: Samoylov Eugene aka Helius (ghelius@gmail.com)

FreshHealth note: This has been partially refactored and extended to fix bugs
and handle use cases not anticipated by the author.

BUGS and TODO:
-- add echo_off feature
-- rewrite history for use more than 256 byte buffer
*/

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "microrl.h"
#ifdef _USE_LIBC_STDIO
#include <stdio.h>
#endif

//#define DBG(...) fprintf(stderr, "\033[33m");fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\033[0m");

#ifdef _USE_HISTORY

//*****************************************************************************
// print buffer content on screen
static inline void print_buf (volatile microrl_t * pThis, volatile char *buf, size_t len)
{
    pThis->print ("\n");
    for (volatile char *p = buf, *e = buf + len + 1; p != e; p++) {
        char b[40];
        if (isprint((size_t)*p)) {
            b[0] = *p;
            b[1] = '\0';
        } else {
            snprintf (b, sizeof(b), "\\x%02x", (unsigned)*p);
        }
        pThis->print (b);
    }
    pThis->print ("\n");
}

//*****************************************************************************
static void hist_save_line (volatile microrl_t * pThis, char * line, size_t len)
{
    char *history = pThis->history;

    if (1 + len > sizeof(microrl_history_t))
        return;

    size_t num_in_use_bytes = 0;
    while (num_in_use_bytes < sizeof(microrl_history_t) && history[num_in_use_bytes] != 0) {
        size_t l = (size_t)history[num_in_use_bytes];
        size_t next_offset = num_in_use_bytes + 1 + l;
        if (l == len && memcmp(&history[num_in_use_bytes + 1], line, l) == 0) {
            if (next_offset == sizeof(microrl_history_t)) {
                history[num_in_use_bytes] = 0x00;
            } else {
                memmove(&history[num_in_use_bytes], &history[next_offset], sizeof(microrl_history_t) - next_offset); // Discard old dup
                history[sizeof(microrl_history_t) - next_offset] = 0x00;
            }
        } else {
            num_in_use_bytes = next_offset;
        }
    }

    while (history[0] != 0 && sizeof(microrl_history_t) - num_in_use_bytes < 1 + len) {
        size_t n = 1 + (size_t)history[0];
        num_in_use_bytes -= n;

        memmove(history, history + n, sizeof(microrl_history_t) - n);
    }

    history[num_in_use_bytes++] = len;
    memcpy (history + num_in_use_bytes, line, len);
    num_in_use_bytes += len;
    if (num_in_use_bytes < sizeof(microrl_history_t)) {
        history[num_in_use_bytes] = 0;
    }

    pThis->cur_hist_offset = 0;
#ifdef _HISTORY_DEBUG
    print_buf (pThis, history, num_in_use_bytes);
#endif
}

//*****************************************************************************
// copy saved line to 'line' and return size of line
static int hist_restore_line (volatile microrl_t * pThis, char * line, int dir)
{
    char *history = pThis->history;

    size_t num_history_entries = 0;
    for (size_t i = 0; i < sizeof(microrl_history_t) && history[i] != 0; ++num_history_entries) {
        i += history[i] + 1;
    }

    if (dir == _HIST_UP) {
        if (pThis->cur_hist_offset >= num_history_entries) {
            return(-1);
        }
        ++pThis->cur_hist_offset;
    } else {
        if (pThis->cur_hist_offset == 0) {
            return(-1);
        }

        --pThis->cur_hist_offset;
        if (pThis->cur_hist_offset == 0) {
            return(0);
        }
    }

    size_t i = 0;
    for (size_t j = num_history_entries; j > pThis->cur_hist_offset; --j) {
        i += 1 + history[i];
    }

    int len = history[i];

    memcpy (line, history + i + 1, len);

    return(len);
}
#endif

//*****************************************************************************
// tokenize cmdline to tkn array and return nmb of token
static int tokenize (volatile microrl_t * pThis, int limit, char const ** tkn_arr)
{
    int num_tokens = 0;
    int offset = 0;
    while (1) {
        if (num_tokens >= _MAX_NUM_TOKENS) {
            return(-1);
        }

        while (pThis->cmdline [offset] == ' ' && offset < limit) {
            offset++;
        }
        if (offset >= limit) {
            return(num_tokens);
        }

        tkn_arr[num_tokens++] = pThis->cmdline + offset;

        while (pThis->cmdline [offset] != ' ' && offset < limit) {
            offset++;
        }
        pThis->cmdline[offset++] = '\0'; // Caller is responsible for saving and restoring this char if it's printable
        if (offset >= limit)
            return(num_tokens);
    }
    return(num_tokens);
}

//********************************************************************************
static void detokenize (volatile microrl_t * pThis) {
    for( volatile char *p = pThis->cmdline, *e = pThis->cmdline + pThis->cmdlen; p != e; ++p) {
        if (*p == '\0')
            *p = ' ';
    }
}


//*****************************************************************************
inline static void terminal_newline (volatile microrl_t * pThis)
{
    pThis->print ("\n");
}

#ifndef _USE_LIBC_STDIO
//*****************************************************************************
// convert 16 bit value to string
// 0 value not supported!!! just make empty string
// Returns pointer to a buffer tail
static char *u16bit_to_str (unsigned int nmb, char * buf)
{
    char tmp_str [6] = {0,};
    int i = 0, j;
    if (nmb <= 0xFFFF) {
        while (nmb > 0) {
            tmp_str[i++] = (nmb % 10) + '0';
            nmb /=10;
        }
        for (j = 0; j < i; ++j)
            *(buf++) = tmp_str [i-j-1];
    }
    *buf = '\0';
    return(buf);
}
#endif


//*****************************************************************************
// Position cursor on command line
static void terminal_place_cursor_at (volatile microrl_t * pThis, unsigned char_pos) // pos is 0...
{
    if (char_pos == 0) {
        pThis->print ("\r");
    } else {
        char str[16] = "\033[";
        char *endstr = &str[2];

#ifdef _USE_LIBC_STDIO
        endstr += snprintf (endstr, 14, "%u", char_pos + 1u);
#else
        endstr = u16bit_to_str (char_pos + 1u, endstr);
#endif
        *(endstr++) = 'G';
        *(endstr++) = '\0';
        pThis->print (str);
    }
}

static void terminal_place_cursor (volatile microrl_t * pThis)
{
    terminal_place_cursor_at (pThis, (unsigned)(strlen(pThis->prompt_str) + _PROMPT_LEN + pThis->cursor));
}

//*****************************************************************************
static void terminal_print_prompt (volatile microrl_t * pthis)
{
    terminal_place_cursor_at( pthis, 0);
    pthis->print (pthis->prompt_str);
    pthis->print (_PROMPT_DEFAULT);
}

//*****************************************************************************
// print cmdline to screen, replace '\0' to whitespace
void terminal_print_line (volatile microrl_t * pthis, int pos)
{
    char nch [] = {0,0};
    int i;
    for (i = pos; i < pthis->cmdlen; i++) {
        nch [0] = pthis->cmdline [i];
        if (nch[0] == '\0')
            nch[0] = ' ';
        pthis->print (nch);
    }
    pthis->print ("\033[K"); // erase to eol

    if (pthis->cursor != pthis->cmdlen) {
        terminal_place_cursor (pthis);
    }
}

//******************************************************************************
void terminal_print_prompt_and_line (volatile microrl_t * pthis)
{
    terminal_print_prompt (pthis);
    terminal_print_line   (pthis, 0);
}

//*****************************************************************************
void microrl_init (volatile microrl_t * pThis, void (*print_) (const char *))
{
    pThis->print = print_;
#ifdef _ENABLE_INIT_PROMPT
    terminal_print_prompt (pThis);
#endif
}

//*****************************************************************************
void microrl_set_complete_callback (volatile microrl_t * pThis, const char ** (*get_completions)(int, const char* const*))
{
    pThis->get_completions = get_completions;
}

//*****************************************************************************
void microrl_set_execute_callback (volatile microrl_t * pThis, void (*execute_)(int, const char* const*))
{
    pThis->execute = execute_;
}

#ifdef _USE_CTLR_C
//*****************************************************************************
void microrl_set_sigint_callback (volatile microrl_t * pThis, void (*sigintf)(void))
{
    pThis->sigint = sigintf;
}
#endif

//*****************************************************************************
void microrl_clear_cmdline (volatile microrl_t * pThis) {
    pThis->cursor = pThis->cmdlen = 0;
}

//*****************************************************************************
#ifdef _USE_HISTORY
static void hist_search (volatile microrl_t * pThis, int dir)
{
    int len = hist_restore_line (pThis, pThis->cmdline, dir);
    if (len >= 0) {
        pThis->cmdline[len] = '\0';
        pThis->cursor = pThis->cmdlen = len;
        terminal_place_cursor_at (pThis, strlen(pThis->prompt_str) + _PROMPT_LEN);
        terminal_print_line (pThis, 0);
    } else {
        pThis->print("\007"); // BEL
    }
}

//*****************************************************************************
static void microrl_execute_cmdline(volatile microrl_t * pThis) {
    char const * tkn_arr[_MAX_NUM_TOKENS];

    int status = tokenize (pThis, pThis->cmdlen, tkn_arr);
    if (status == -1) {
        pThis->print ("ERROR: too many tokens\n");
    }
    pThis->cmdlen = 0;
    pThis->cursor = 0;
    if (status > 0 && pThis->execute != NULL)
        pThis->execute (status, tkn_arr);
}
//*****************************************************************************
int microrl_repeat_most_recent_history(volatile microrl_t * pThis) {
    int len;

    do {
        len = hist_restore_line (pThis, pThis->cmdline, _HIST_UP);
    } while (len >= 0 && strncmp(pThis->cmdline, "repeat ", 7) == 0);

    pThis->cur_hist_offset = 0;

    if (len >= 0) {
        pThis->cmdline[len] = '\0';
        pThis->cursor = pThis->cmdlen = len;

        microrl_execute_cmdline (pThis);

        return (1);
    } else {
        pThis->print("\007"); // BEL
        return (0);
    }
}
#endif

//*****************************************************************************
static void microrl_process_DEL(volatile microrl_t * pThis) {
    if (pThis->cursor < pThis->cmdlen) {
        memmove (pThis->cmdline + pThis->cursor,
                 pThis->cmdline + (pThis->cursor + 1),
                 pThis->cmdlen  - (pThis->cursor + 1));

        pThis->cmdlen--;
        pThis->cmdline [pThis->cmdlen] = '\0';

        terminal_place_cursor (pThis);
        terminal_print_line (pThis, pThis->cursor);
    }
}

#ifdef _USE_ESC_SEQ
//*****************************************************************************
// handling escape sequences
static int escape_process (volatile microrl_t * pThis, char ch)
{
    // printf("char: %d %s\n\r", ch, ch);
    if (ch == '[') {
        pThis->escape_seq = -1;
        return(0);
    } else if (pThis->escape_seq == -1) {
        if (ch == 'A') {
#ifdef _USE_HISTORY
            hist_search (pThis, _HIST_UP);
#endif
            return(1);
        } else if (ch == 'B') {
#ifdef _USE_HISTORY
            hist_search (pThis, _HIST_DOWN);
#endif
            return(1);
        } else if (ch == 'C') {
            if (pThis->cursor < pThis->cmdlen) {
                pThis->cursor++;
                terminal_place_cursor (pThis);
            }
            return(1);
        } else if (ch == 'D') {
            if (pThis->cursor > 0) {
                pThis->cursor--;
                terminal_place_cursor (pThis);
            }
            return(1);
        } else if (ch >= '0' && ch <= '9') {
            pThis->escape_seq = ch - '0';
            return(0);
        }
    } else if (ch >= '0' && ch <= '9') {
        pThis->escape_seq *= 10;
        pThis->escape_seq += ch - '0';
    } else if (ch == '~') {
        switch (pThis->escape_seq) {
            case 1: // HOME key
            case 7: // HOME key
                pThis->cursor = 0;
                terminal_place_cursor (pThis);
                break;
            case 3: // Delete
                microrl_process_DEL (pThis);
                break;

            case 4: // END key
            case 8: // END key
                pThis->cursor = pThis->cmdlen;
                terminal_place_cursor (pThis);
                break;
        }
    }

    /* unknown escape sequence, stop */
    return(1);
}
#endif

//*****************************************************************************
// insert len char of text at cursor position
static int microrl_insert_text (volatile microrl_t * pThis, char * text, int len)
{
    int i;
    if (pThis->cmdlen + len < _COMMAND_LINE_LEN) {
        memmove (pThis->cmdline + pThis->cursor + len,
                         pThis->cmdline + pThis->cursor,
                         pThis->cmdlen - pThis->cursor);
        for (i = 0; i < len; i++) {
            pThis->cmdline [pThis->cursor + i] = text [i];
        }
        pThis->cursor += len;
        pThis->cmdlen += len;
        return(true);
    }
    return(false);
}

#ifdef _USE_COMPLETE

//*****************************************************************************
static int common_len (const char ** arr)
{
    size_t i;
    size_t j;
    char *shortest = arr[0];
    size_t shortlen = strlen(shortest);

    for (i = 1; arr[i] != NULL; ++i)
        if (strlen(arr[i]) < shortlen) {
            shortest = arr[i];
            shortlen = strlen(shortest);
        }

    for (i = 0; i < shortlen; ++i)
        for (j = 0; arr[j] != NULL; ++j)
            if (tolower(shortest[i]) != tolower(arr[j][i]))
                return(i);

    return(i);
}

//*****************************************************************************
static void microrl_perform_completion (volatile microrl_t * pThis)
{
    if (pThis->get_completions == NULL) // callback was not set
        return;

    char saved = pThis->cmdline[pThis->cursor];

    char const * tkn_arr[_MAX_NUM_TOKENS];
    int status = tokenize (pThis, pThis->cursor, tkn_arr);

    size_t last_token_len = strlen(tkn_arr[status - 1]);

    const char **compl_token = NULL;
    if (status > 0) {
        if (saved == ' ' && status < _MAX_NUM_TOKENS) {
            tkn_arr[status++] = "";
        }
        compl_token = pThis->get_completions (status, tkn_arr);
    }
    detokenize (pThis);
    pThis->cmdline[pThis->cursor] = saved;

    if (compl_token == NULL || compl_token[0] == NULL) {
        pThis->print("\007"); // BEL
        return;
    }

    int i = 0;
    int len;

    int print_from_offset = pThis->cursor;

    if (compl_token[1] == NULL) {
        len = strlen (compl_token[0]);
    } else {
        len = common_len (compl_token);
        if (len == pThis->cursor) {
            terminal_newline (pThis);
            while (compl_token [i] != NULL) {
                pThis->print (compl_token[i]);
                pThis->print ("\n");
                i++;
            }
            terminal_newline (pThis);
            terminal_print_prompt (pThis);
            print_from_offset = 0;
        }
    }

    if (len > 0) {
        microrl_insert_text (
            pThis,
            compl_token[0] + last_token_len,
            len - last_token_len
        );
    }
    terminal_print_line (pThis, print_from_offset);
}
#endif

//*****************************************************************************
void new_line_handler(volatile microrl_t * pThis){
    terminal_newline (pThis);
#ifdef _USE_HISTORY
    if (pThis->cmdlen > 0)
        hist_save_line (pThis, pThis->cmdline, pThis->cmdlen);
    pThis->cur_hist_offset = 0;
#endif
    microrl_execute_cmdline (pThis);
    terminal_print_prompt(pThis);
}

//*****************************************************************************

void microrl_insert_char (volatile microrl_t * pThis, int ch)
{
#ifdef _USE_ESC_SEQ
    if (pThis->escape) {
        if (escape_process(pThis, ch)) {
            pThis->escape     = 0;
            pThis->escape_seq = 0;
        }
    } else {
#endif
        switch (ch) {
            //-----------------------------------------------------
#ifdef _ENDL_CR
            case KEY_CR:
                new_line_handler(pThis);
            break;
            case KEY_LF:
            break;
#elif defined(_ENDL_CRLF)
            case KEY_CR:
                pThis->tmpch = KEY_CR;
            break;
            case KEY_LF:
            if (pThis->tmpch == KEY_CR)
                new_line_handler(pThis);
            break;
#elif defined(_ENDL_LFCR)
            case KEY_LF:
                pThis->tmpch = KEY_LF;
            break;
            case KEY_CR:
            if (pThis->tmpch == KEY_LF)
                new_line_handler(pThis);
            break;
#else
            case KEY_CR:
            break;
            case KEY_LF:
                new_line_handler(pThis);
            break;
#endif
            //-----------------------------------------------------
#ifdef _USE_COMPLETE
            case KEY_HT:
                microrl_perform_completion (pThis);
            break;
#endif
            //-----------------------------------------------------
            case KEY_ESC:
#ifdef _USE_ESC_SEQ
                pThis->escape = 1;
#endif
            break;
            //-----------------------------------------------------
            case KEY_NAK: // ^U
                if (pThis->cursor > 0) {
                    memmove (pThis->cmdline,
                             pThis->cmdline + pThis->cursor,
                             pThis->cmdlen  - pThis->cursor);
                    pThis->cmdlen -= pThis->cursor;
                    pThis->cursor = 0;
                    pThis->cmdline[pThis->cmdlen] = 0;
                    terminal_place_cursor_at (pThis, strlen(pThis->prompt_str) + _PROMPT_LEN);
                    terminal_print_line (pThis, 0);
                }
            break;
            //-----------------------------------------------------
            case KEY_VT:  // ^K
                pThis->print ("\033[K");
                pThis->cmdlen = pThis->cursor;
            break;
            //-----------------------------------------------------
            case KEY_ENQ: // ^E
                pThis->cursor = pThis->cmdlen;
                terminal_place_cursor (pThis);
            break;
            //-----------------------------------------------------
            case KEY_SOH: // ^A
                pThis->cursor = 0;
                terminal_place_cursor (pThis);
            break;
            //-----------------------------------------------------
            case KEY_ACK: // ^F
            if (pThis->cursor < pThis->cmdlen) {
                pThis->cursor++;
                terminal_place_cursor (pThis);
            }
            break;
            //-----------------------------------------------------
            case KEY_STX: // ^B
            if (pThis->cursor) {
                pThis->cursor--;
                terminal_place_cursor (pThis);
            }
            break;
            //-----------------------------------------------------
            case KEY_DLE: //^P
#ifdef _USE_HISTORY
            hist_search (pThis, _HIST_UP);
#endif
            break;
            //-----------------------------------------------------
            case KEY_SO: //^N
#ifdef _USE_HISTORY
            hist_search (pThis, _HIST_DOWN);
#endif
            break;
            //-----------------------------------------------------
            case KEY_DEL: // Backspace
            case KEY_BS:  // ^H Backspace on some terminals
                if (pThis->cursor > 0) {
                    if (pThis->cursor != pThis->cmdlen) {
                        memmove (pThis->cmdline + pThis->cursor - 1,
                                 pThis->cmdline + pThis->cursor,
                                 pThis->cmdlen - pThis->cursor);
                    }
                    pThis->cursor--;
                    pThis->cmdlen--;
                    pThis->cmdline [pThis->cmdlen] = '\0';

                    terminal_place_cursor (pThis);
                    terminal_print_line (pThis, pThis->cursor);
                }
            break;
            //-----------------------------------------------------
            case KEY_DC2: // ^R
                terminal_print_prompt (pThis);
                terminal_print_line   (pThis, 0);
            break;
            //-----------------------------------------------------
#ifdef _USE_CTLR_C
            case KEY_ETX:
                pThis->cursor = 0;
                pThis->cmdlen = 0;
                if (pThis->sigint != NULL)
                    pThis->sigint();
                terminal_print_prompt (pThis);
            break;
#endif
            //-----------------------------------------------------
            default:
            if (((ch == ' ') && (pThis->cmdlen == 0)) || IS_CONTROL_CHAR(ch))
                break;
            if (microrl_insert_text (pThis, (char*)&ch, 1))
                terminal_print_line (pThis, pThis->cursor-1);

            break;
        }
#ifdef _USE_ESC_SEQ
    }
#endif
}
