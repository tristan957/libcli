/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#ifndef LIBCLI_OUTPUT_H
#define LIBCLI_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* ECMA-48 5th Edition: 8.3.117 SGR - Select Graphic Rendition
 *
 * https://www.ecma-international.org/wp-content/uploads/ECMA-48_5th_edition_june_1991.pdf
 */
#define CLI_DEFAULT                  "\033[0m"
#define CLI_BOLD                     "\033[1m"
#define CLI_FAINT                    "\033[2m"
#define CLI_ITALICIZED               "\033[3m"
#define CLI_SINGLY_UNDERLINED        "\033[4m"
#define CLI_SLOWLY_BLINK             "\033[5m"
#define CLI_RAPIDLY_BLINK            "\033[6m"
#define CLI_NEGATIVE_IMAGE           "\033[7m"
#define CLI_CONCEALED_CHARACTERS     "\033[8m"
#define CLI_CROSSED_OUT              "\033[9m"
#define CLI_PRIMARY_FONT             "\033[10m"
#define CLI_FIRST_ALTERNATIVE_FONT   "\033[11m"
#define CLI_SECOND_ALTERNATIVE_FONT  "\033[12m"
#define CLI_THIRD_ALTERNATIVE_FONT   "\033[13m"
#define CLI_FOURTH_ALTERNATIVE_FONT  "\033[14m"
#define CLI_FIFTH_ALTERNATIVE_FONT   "\033[15m"
#define CLI_SIXTH_ALTERNATIVE_FONT   "\033[16m"
#define CLI_SEVENTH_ALTERNATIVE_FONT "\033[17m"
#define CLI_EIGHTH_ALTERNATIVE_FONT  "\033[18m"
#define CLI_NINTH_ALTERNATIVE_FONT   "\033[19m"
#define CLI_FRAKTUR                  "\033[20m"
#define CLI_DOUBLY_UNDERLINED        "\033[21m"
#define CLI_NORMAL_INTENSITY         "\033[22m"
#define CLI_NOT_ITALICIZED           "\033[23m"
#define CLI_NOT_UNDERLINED           "\033[24m"
#define CLI_STEADY                   "\033[25m"
#define CLI_POSITIVE_IMAGE           "\033[27m"
#define CLI_REVEALED_CHARACTERS      "\033[28m"
#define CLI_NOT_CROSSED_OUT          "\033[29m"
#define CLI_BLACK_FG                 "\033[30m"
#define CLI_RED_FG                   "\033[31m"
#define CLI_GREEN_FG                 "\033[32m"
#define CLI_YELLOW_FG                "\033[33m"
#define CLI_BLUE_FG                  "\033[34m"
#define CLI_MAGENTA_FG               "\033[35m"
#define CLI_CYAN_FG                  "\033[36m"
#define CLI_WHITE_FG                 "\033[37m"
#define CLI_DEFAULT_FG               "\033[39m"
#define CLI_BLACK_BG                 "\033[40m"
#define CLI_RED_BG                   "\033[41m"
#define CLI_GREEN_BG                 "\033[42m"
#define CLI_YELLOW_BG                "\033[43m"
#define CLI_BLUE_BG                  "\033[44m"
#define CLI_MAGENTA_BG               "\033[45m"
#define CLI_CYAN_BG                  "\033[46m"
#define CLI_WHITE_BG                 "\033[47m"
#define CLI_DEFAULT_BG               "\033[49m"
#define CLI_FRAMED                   "\033[51m"
#define CLI_ENCIRCLED                "\033[52m"
#define CLI_OVERLINED                "\033[53m"
#define CLI_NOT_FRAMED_ENCIRCLED     "\033[54m"
#define CLI_NOT_OVERLINED            "\033[55m"
#define CLI_IDEOGRAM_UNDERLINE       "\033[61m"
#define CLI_IDEOGRAM_OVERLINE        "\033[62m"
#define CLI_IDEOGRAM_DOUBLE_OVERLINE "\033[63m"
#define CLI_IDEOGRAM_STRESS_MARKING  "\033[64m"
#define CLI_CANCEL_IDEOGRAM          "\033[65m"

enum cli_justify {
    CLI_JUSTIFY_LEFT,
    CLI_JUSTIFY_RIGHT,
};

int
cli_error(const char * const fmt, ...);

#define cli_fatalx(exit_code, fmt, ...) \
    do {                                \
        cli_error(fmt, __VA_ARGS__);    \
        exit(exit_code);                \
    } while (0)

#define cli_fatal(fmt, ...) cli_fatalx(EXIT_FAILURE, fmt, __VA_ARGS__)

int
cli_print_table(
    FILE *stream,
    size_t nrow,
    size_t ncol,
    const char * const *headers,
    const char * const *values,
    const enum cli_justify *justify,
    const bool *enabled);

#endif
