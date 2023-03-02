/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libcli/output.h>
#include <libcli/program.h>

#define COLUMN_SEP "  "

int
cli_error(const char * const fmt, ...)
{
    va_list ap;
    int printed = 0;

    if (!fmt)
        return -1;

    va_start(ap, fmt);
    if (cli_program_name_short) {
        char buf[512];

        printed += fputs(cli_program_name_short, stderr);
        snprintf(buf, sizeof(buf), ": %s\n", fmt);
        printed += vfprintf(stderr, buf, ap);
    } else {
        printed += vfprintf(stderr, fmt, ap);
    }
    va_end(ap);

    return printed;
}

int
cli_print_table(
    FILE * const stream,
    size_t nrow,
    size_t ncol,
    const char * const * const headers,
    const char * const * const values,
    const enum cli_justify * const justify,
    const bool * const enabled)
{
    int ch;
    int printed = 0;
    size_t *longest;

    if (!ncol || !headers || !values)
        return -1;

    longest = malloc(ncol * sizeof(*longest));
    if (!longest)
        return 0;

    for (size_t c = 0; c < ncol; c++) {
        size_t max;

        max = strlen(headers[c]);

        for (size_t r = 0; r < nrow; r++) {
            const size_t n = strlen(values[r * ncol + c]);

            if (n > max)
                max = n;
        }

        longest[c] = max;
    }

    for (size_t c = 0; c < ncol; c++) {
        const char *fmt = "";
        enum cli_justify just;

        if (enabled && !enabled[c])
            continue;

        just = justify ? justify[c] : CLI_JUSTIFY_LEFT;
        switch (just) {
        case CLI_JUSTIFY_LEFT:
            fmt = "%s%-*s";
            break;
        case CLI_JUSTIFY_RIGHT:
            fmt = "%s%*s";
            break;
        }

        printed += fprintf(stream, fmt, c == 0 ? "" : COLUMN_SEP, (int)longest[c], headers[c]);
    }

    ch = fputc('\n', stream);
    if (ch != EOF)
        printed++;

    for (size_t r = 0; r < nrow; r++) {
        for (size_t c = 0; c < ncol; c++) {
            size_t sub = 0;
            const char *value;
            const char *fmt = "";
            enum cli_justify just;

            if (enabled && !enabled[c])
                continue;

            value = values[r * ncol + c];
            just = justify ? justify[c] : CLI_JUSTIFY_LEFT;
            switch (just) {
            case CLI_JUSTIFY_LEFT:
                fmt = "%s%-*s";
                break;
            case CLI_JUSTIFY_RIGHT:
                fmt = "%s%*s";
                break;
            }

            if (c == ncol - 1 && just == CLI_JUSTIFY_LEFT)
                sub = longest[c] - strlen(value);

            printed +=
                fprintf(stream, fmt, c == 0 ? "" : COLUMN_SEP, (int)(longest[c] - sub), value);
        }

        ch = fputc('\n', stream);
        if (ch != EOF)
            printed++;
    }

    free(longest);

    return printed;
}
