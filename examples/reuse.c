/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include <libcli/parser.h>

#include "util.h"

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

static void
error(const char * const argv_0, const char * const fmt, ...)
{
    va_list ap;
    char buf[256];

    fputs(argv_0, stderr);
    snprintf(buf, sizeof(buf), ": %s\n", fmt);
    va_start(ap, fmt);
    vfprintf(stderr, buf, ap);
    va_end(ap);
}

int
main(const int argc, char *argv[])
{
    int rc;
    const char *progname;

    progname = strrchr(argv[0], PATH_SEP);
    if (progname != argv[0])
        progname++;

    rc = cli_add_options(&root, NELEM(root_options), root_options);
    if (rc) {
        error(progname, "Failed to add root options: %s (%d)", strerror(rc), rc);
        return EX_DATAERR;
    }

    rc = cli_add_subcommands(&root, NELEM(root_subcommands), root_subcommands);
    if (rc) {
        error(progname, "Failed to add root subcommands: %s (%d)", strerror(rc), rc);
        return EX_DATAERR;
    }

    rc = cli_parse(&root, argc, argv);
    if (rc)
        return rc;

    return 0;
}
