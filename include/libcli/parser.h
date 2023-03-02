/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#ifndef LIBCLI_PARSER_H
#define LIBCLI_PARSER_H

#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>

#include <sys/queue.h>

#include <merr.h>

struct cli;

typedef void
cli_callback(const struct cli *cli, int *exit_code, void *ctx);

enum cli_has_arg {
    CLI_HAS_ARG_NONE,
    CLI_HAS_ARG_REQUIRED,
#ifndef CLI_NO_OPTIONAL_ARGUMENT
    CLI_HAS_ARG_OPTIONAL,
#endif
};

enum cli_type {
    CLI_TYPE_BOOL,
    CLI_TYPE_UCHAR,
    CLI_TYPE_USHORT,
    CLI_TYPE_UINT,
    CLI_TYPE_ULONG,
    CLI_TYPE_ULONGLONG,
    CLI_TYPE_U8,
    CLI_TYPE_U16,
    CLI_TYPE_U32,
    CLI_TYPE_U64,
    CLI_TYPE_CHAR,
    CLI_TYPE_SHORT,
    CLI_TYPE_INT,
    CLI_TYPE_LONG,
    CLI_TYPE_LONGLONG,
    CLI_TYPE_I8,
    CLI_TYPE_I16,
    CLI_TYPE_I32,
    CLI_TYPE_I64,
    CLI_TYPE_FLOAT,
    CLI_TYPE_DOUBLE,
    CLI_TYPE_LONGDOUBLE,
    CLI_TYPE_STRING,
};

enum cli_action {
    CLI_ACTION_HELP,
    CLI_ACTION_ACCUMULATE,
    CLI_ACTION_STORE,
};

struct cli_option {
    char shrt;
#ifndef CLI_NO_GETOPT_LONG
    const char *lng;
#endif
    const char *description;
    enum cli_has_arg argument;
    enum cli_type type;
    enum cli_action action;
    void *data;
    SLIST_ENTRY(cli_option) entry;
};

struct cli_argument {
    const char *name;
    const char *description;
    enum cli_type type;
    SLIST_ENTRY(cli_argument) entry;
};

struct cli {
    const char *name;
    const char *description;
    cli_callback *callback;
    void *ctx;
    SLIST_HEAD(options, cli_option) options;
    SLIST_HEAD(subcommands, cli) subcommands;
    SLIST_HEAD(arguments, cli_argument) arguments;
    SLIST_ENTRY(cli) entry;
};

merr_t
cli_add_argument(struct cli *cli, struct cli_argument *argument);

merr_t
cli_add_arguments(struct cli *cli, size_t optionc, struct cli_argument *argumentv);

merr_t
cli_add_option(struct cli *cli, struct cli_option *option);

merr_t
cli_add_options(struct cli *cli, size_t optionc, struct cli_option *optionv);

merr_t
cli_add_subcommand(struct cli *cli, struct cli *subcommand);

merr_t
cli_add_subcommands(struct cli *cli, size_t subcommandc, struct cli *subcommandv);

void
cli_init(struct cli *cli);

merr_t
cli_parse(const struct cli *cli, int argc, char * const *argv, int *exit_code);

#endif
