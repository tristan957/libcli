/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include "util.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include <libcli/output.h>
#include <libcli/parser.h>
#include <libcli/program.h>

#define NELEM(arr) (sizeof(arr) / sizeof(arr[0]))

static struct cli root = {
    .name = "reuse",
    .description = "reuse is a tool for compliance with the REUSE recommendations. See\n"
                   "<https://reuse.software/> for more information, and\n"
                   "<https://reuse.readthedocs.io/> for the online documentation.\n"
                   "\n"
                   "This version of reuse is compatible with version 3.0 of the REUSE\n"
                   "Specification.\n"
                   "\n"
                   "Support the FSFE's work:\n"
                   "\n"
                   "  Donations are critical to our strength and autonomy. They enable us to\n"
                   "  continue working for Free Software wherever necessary. Please consider\n"
                   "  making a donation at <https://fsfe.org/donate/>.",
};

static struct cli root_subcommands[] = { {
                                             .name = "init",
                                             .description = "initialize REUSE project",
                                         },
                                         {
                                             .name = "longlong",
                                         } };

static struct cli_option root_options[] = { {
                                                .shrt = 'h',
#ifndef CLI_NO_GETOPT_LONG
                                                .lng = "help",
#endif
                                                .action = CLI_ACTION_HELP,
                                                .description = "Print this help output",
                                            },
                                            {
                                                .shrt = 'l',
#ifndef CLI_NO_GETOPT_LONG
                                                .lng = "ll",
#endif
                                                .description = "Long argument",
                                            } };

static struct cli_argument init_arguments[] = {
    { .name = "file", .description = "hello world" },
    { .name = "other thing" },
};

int
main(const int argc, char * const argv[])
{
    merr_t err;
    int exit_code;
    char buf[256];

    cli_set_program_name(argv[0]);

    err = cli_add_options(&root, NELEM(root_options), root_options);
    if (err) {
        merr_strerror(err, buf, sizeof(buf));
        cli_error("Failed to add root options: %s", buf);
        return EX_DATAERR;
    }

    err = cli_add_subcommands(&root, NELEM(root_subcommands), root_subcommands);
    if (err) {
        merr_strerror(err, buf, sizeof(buf));
        cli_error("Failed to add root subcommands: %s", buf);
        return EX_DATAERR;
    }

    err = cli_add_arguments(&root_subcommands[0], NELEM(init_arguments), init_arguments);
    if (err) {
        merr_strerror(err, buf, sizeof(buf));
        cli_error("Failed to add init arguments: %s", buf);
        return EX_DATAERR;
    }

    err = cli_parse(&root, argc, argv, &exit_code);
    assert(!err);

    return exit_code;
}
