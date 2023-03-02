/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include "util.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include <sys/queue.h>

#include <libcli/output.h>
#include <libcli/parser.h>
#include <libcli/program.h>

#define TAB "  "

int
cli_add_option(struct cli * const cli, struct cli_option * const option)
{
    struct cli_option *o;
    struct cli_option *prev;

    if (!cli || !option)
        return EINVAL;

    if (SLIST_EMPTY(&cli->options)) {
        SLIST_INSERT_HEAD(&cli->options, option, entry);

        return 0;
    }

    prev = SLIST_FIRST(&cli->options);
    SLIST_FOREACH(o, &cli->options, entry) {
        if (o == option || o->shrt == option->shrt)
            return ENOTUNIQ;

        if (o->shrt > option->shrt)
            break;

        /* We want to continue looping through the rest of the options to verify
         * other preconditions.
         */
        if (!prev)
            prev = o;
    }

    SLIST_INSERT_AFTER(prev, option, entry);

    return 0;
}

int
cli_add_options(struct cli *cli, const size_t optionc, struct cli_option * const optionv)
{
    if (!cli || !optionv)
        return EINVAL;

    for (size_t i = 0; i < optionc; i++) {
        int rc;

        rc = cli_add_option(cli, optionv + i);
        if (rc)
            return rc;
    }

    return 0;
}

int
cli_add_subcommand(struct cli *cli, struct cli * const subcommand)
{
    struct cli *c;
    struct cli *prev;

    if (!cli || !subcommand)
        return EINVAL;

    if (cli == subcommand)
        return EINVAL;

    if (SLIST_EMPTY(&cli->subcommands)) {
        SLIST_INSERT_HEAD(&cli->subcommands, subcommand, entry);

        return 0;
    }

    prev = SLIST_FIRST(&cli->subcommands);
    SLIST_FOREACH(c, &cli->subcommands, entry) {
        int rc;

        rc = strcmp(c->name, subcommand->name);
        if (c == subcommand || rc == 0)
            return ENOTUNIQ;

        if (rc > 0)
            break;

        /* We want to continue looping through the rest of the options to
         * verify other preconditions.
         */
        if (!prev)
            prev = c;
    }

    SLIST_INSERT_AFTER(prev, subcommand, entry);

    return 0;
}

int
cli_add_subcommands(struct cli *cli, size_t subcommandc, struct cli * const subcommandv)
{
    if (!cli || !subcommandv)
        return EINVAL;

    for (size_t i = 0; i < subcommandc; i++) {
        int rc;

        rc = cli_add_subcommand(cli, subcommandv + i);
        if (rc)
            return rc;
    }

    return 0;
}

void
cli_init(struct cli * const cli)
{
    if (!cli)
        return;

    SLIST_INIT(&cli->options);
}

static size_t
cli_get_num_options(const struct cli * const cli)
{
    size_t i = 0;
    const struct cli_option *o;

    assert(cli);

    SLIST_FOREACH(o, &cli->options, entry)
        i++;

    return i;
}

static const struct cli_option *
cli_get_option(const struct cli * const cli, const char c)
{
    const struct cli_option *o;

    assert(cli);

    SLIST_FOREACH(o, &cli->options, entry) {
        if (c == o->shrt)
            return o;
    }

    return NULL;
}

static void
cli_action_help(const struct cli * const cli, FILE * const output)
{
    assert(cli);
    assert(output);

    fprintf(output, "Usage: %s", cli_program_name);
    if (!SLIST_EMPTY(&cli->options))
        fprintf(output, " [OPTIONS]...");
    fputc('\n', output);
    if (cli->description)
        fprintf(output, "\n%s\n", cli->description);
    if (!SLIST_EMPTY(&cli->options)) {
        size_t max_width = 0;
        const struct cli_option *o;

        fprintf(output, "\nOptions:\n");

        SLIST_FOREACH(o, &cli->options, entry) {
            size_t width = 0;

#ifndef CLI_NO_GETOPT_LONG
            if (o->lng)
                width += strlen(o->lng);
#endif

            switch (o->argument) {
            case CLI_ARGUMENT_NONE:
                break;
            case CLI_ARGUMENT_REQUIRED:
                width += 3;
                break;
#ifndef CLI_NO_OPTIONAL_ARGUMENT
            case CLI_ARGUMENT_OPTIONAL:
                width += 5;
                break;
#endif
            }

            if (width > max_width)
                max_width = width;
        }

        SLIST_FOREACH(o, &cli->options, entry) {
#ifndef CLI_NO_GETOPT_LONG
            const char *arg_str = "";

            switch (o->argument) {
            case CLI_ARGUMENT_NONE:
                break;
            case CLI_ARGUMENT_REQUIRED:
                arg_str = " arg";
                break;
#ifndef CLI_NO_OPTIONAL_ARGUMENT
            case CLI_ARGUMENT_OPTIONAL:
                arg_str = " (arg)";
#endif
            }
#endif

            fprintf(output, TAB " -%c", o->shrt);
#ifndef CLI_NO_GETOPT_LONG
            if (o->lng)
                fprintf(output, ", --%s%*s", o->lng, (int)(max_width - strlen(o->lng)), arg_str);
#endif
            if (o->description)
                fprintf(output, TAB "%s", o->description);
            fputc('\n', output);
        }
    }

    if (!SLIST_EMPTY(&cli->subcommands)) {
        size_t max_width = 0;
        const struct cli *c;

        fprintf(output, "\nSubcommands:\n");

        SLIST_FOREACH(c, &cli->subcommands, entry) {
            size_t width;

            width = strlen(c->name);
            if (width > max_width)
                max_width = width;
        }

        SLIST_FOREACH(c, &cli->subcommands, entry) {
            fprintf(output, TAB "%-*s", (int)max_width, c->name);
            if (c->description)
                fprintf(output, TAB "%s", c->description);
            fputc('\n', output);
        }
    }
}

static int
parse_bool(const char * const arg, bool * const value)
{
    assert(arg);
    assert(value);

    if (strcmp(arg, "true") == 0) {
        *value = true;
    } else if (strcmp(arg, "false") == 0) {
        *value = false;
    } else {
        return EX_USAGE;
    }

    return 0;
}

static int
parse_base(const char * const arg)
{
    int base;

    assert(arg);

    if (strstr(arg, "0x") == arg) {
        base = 16;
    } else if (strstr(arg, "0o") == arg) {
        base = 8;
    } else if (strstr(arg, "0b") == arg) {
        base = 2;
    } else {
        base = 10;
    }

    return base;
}

static int
parse_uint(const char * const arg, const unsigned long long max, unsigned long long * const value)
{
    int base;

    assert(arg);
    assert(value);

    base = parse_base(arg);

    errno = 0;
    *value = strtoull(arg + (base == 10 ? 0 : 2), NULL, base);
    if (errno == ERANGE || *value > max)
        return EX_USAGE;

    return 0;
}

static int
parse_int(const char * const arg, const long long min, const long long max, long long * const value)
{
    int base;

    assert(arg);
    assert(value);

    base = parse_base(arg);

    errno = 0;
    *value = strtoll(arg + (base == 10 ? 0 : 2), NULL, base);
    if (errno == ERANGE || *value < min || *value > max)
        return EX_USAGE;

    return 0;
}

static int
cli_action_store(
    const struct cli * const cli,
    const struct cli_option * const option,
    const char *arg)
{
    int rc;
    union {
        long long s;
        unsigned long long u;
        float f;
        double d;
        long double ld;
    } value;

    (void)cli;

    assert(cli);
    assert(option);

    switch (option->type) {
    case CLI_TYPE_BOOL:
        if (arg) {
            rc = parse_bool(arg, option->data);
            if (rc)
                return rc;
        } else {
            *(bool *)option->data = true;
        }
        break;
    case CLI_TYPE_UCHAR:
        rc = parse_uint(arg, UCHAR_MAX, &value.u);
        if (rc)
            return rc;
        *(unsigned char *)option->data = value.u;
        break;
    case CLI_TYPE_USHORT:
        rc = parse_uint(arg, USHRT_MAX, &value.u);
        if (rc)
            return rc;
        *(unsigned short *)option->data = value.u;
        break;
    case CLI_TYPE_UINT:
        rc = parse_uint(arg, UINT_MAX, &value.u);
        if (rc)
            return rc;
        *(unsigned int *)option->data = value.u;
        break;
    case CLI_TYPE_ULONG:
        rc = parse_uint(arg, ULONG_MAX, &value.u);
        if (rc)
            return rc;
        *(unsigned long *)option->data = value.u;
        break;
    case CLI_TYPE_ULONGLONG:
        rc = parse_uint(arg, ULLONG_MAX, &value.u);
        if (rc)
            return rc;
        *(unsigned long long *)option->data = value.u;
        break;
    case CLI_TYPE_U8:
        rc = parse_uint(arg, UINT8_MAX, &value.u);
        if (rc)
            return rc;
        *(uint8_t *)option->data = value.u;
        break;
    case CLI_TYPE_U16:
        rc = parse_uint(arg, UINT16_MAX, &value.u);
        if (rc)
            return rc;
        *(uint16_t *)option->data = value.u;
        break;
    case CLI_TYPE_U32:
        rc = parse_uint(arg, UINT32_MAX, &value.u);
        if (rc)
            return rc;
        *(uint32_t *)option->data = value.u;
        break;
    case CLI_TYPE_U64:
        rc = parse_uint(arg, UINT64_MAX, &value.u);
        if (rc)
            return rc;
        *(uint64_t *)option->data = value.u;
        break;
    case CLI_TYPE_CHAR:
        rc = parse_int(arg, CHAR_MIN, CHAR_MAX, &value.s);
        if (rc)
            return rc;
        *(char *)option->data = value.s;
        break;
    case CLI_TYPE_SHORT:
        rc = parse_int(arg, SHRT_MIN, SHRT_MAX, &value.s);
        if (rc)
            return rc;
        *(short *)option->data = value.s;
        break;
    case CLI_TYPE_INT:
        rc = parse_int(arg, INT_MIN, INT_MAX, &value.s);
        if (rc)
            return rc;
        *(int *)option->data = value.s;
        break;
    case CLI_TYPE_LONG:
        rc = parse_int(arg, LONG_MIN, LONG_MAX, &value.s);
        if (rc)
            return rc;
        *(long *)option->data = value.s;
        break;
    case CLI_TYPE_LONGLONG:
        rc = parse_int(arg, LLONG_MIN, LLONG_MAX, &value.s);
        if (rc)
            return rc;
        *(long long *)option->data = value.s;
        break;
    case CLI_TYPE_I8:
        rc = parse_int(arg, INT8_MIN, INT8_MAX, &value.s);
        if (rc)
            return rc;
        *(uint8_t *)option->data = value.s;
        break;
    case CLI_TYPE_I16:
        rc = parse_int(arg, INT16_MIN, INT16_MAX, &value.s);
        if (rc)
            return rc;
        *(uint16_t *)option->data = value.s;
        break;
    case CLI_TYPE_I32:
        rc = parse_int(arg, INT32_MIN, INT32_MAX, &value.s);
        if (rc)
            return rc;
        *(uint32_t *)option->data = value.s;
        break;
    case CLI_TYPE_I64:
        rc = parse_int(arg, INT64_MIN, INT64_MAX, &value.s);
        if (rc)
            return rc;
        *(uint64_t *)option->data = value.s;
        break;
    case CLI_TYPE_FLOAT:
        value.f = strtof(arg, NULL);
        if (errno == ERANGE)
            return EX_USAGE;
        *(float *)option->data = value.f;
        break;
    case CLI_TYPE_DOUBLE:
        value.d = strtod(arg, NULL);
        if (errno == ERANGE)
            return EX_USAGE;
        *(double *)option->data = value.d;
        break;
    case CLI_TYPE_LONGDOUBLE:
        value.ld = strtold(arg, NULL);
        if (errno == ERANGE)
            return EX_USAGE;
        *(long double *)option->data = value.ld;
        break;
    case CLI_TYPE_STRING:
        *(const char **)option->data = arg;
        break;
    }

    return 0;
}

static int
cli_action_accumulate(
    const struct cli * const cli,
    const struct cli_option * const option,
    const char * const arg)
{
    int rc;
    union {
        long long s;
        unsigned long long u;
        float f;
        double d;
        long double ld;
    } value = { 1 };

    (void)cli;

    assert(cli);
    assert(option);

    switch (option->type) {
    case CLI_TYPE_BOOL:
        *(bool *)option->data ^= true;
        break;
    case CLI_TYPE_UCHAR:
        if (arg) {
            rc = parse_uint(arg, UCHAR_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(unsigned char *)option->data += value.u;
        break;
    case CLI_TYPE_USHORT:
        if (arg) {
            rc = parse_uint(arg, USHRT_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(unsigned short *)option->data += value.u;
        break;
    case CLI_TYPE_UINT:
        if (arg) {
            rc = parse_uint(arg, UINT_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(unsigned int *)option->data += value.u;
        break;
    case CLI_TYPE_ULONG:
        if (arg) {
            rc = parse_uint(arg, ULONG_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(unsigned long *)option->data += value.u;
        break;
    case CLI_TYPE_ULONGLONG:
        if (arg) {
            rc = parse_uint(arg, ULLONG_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(unsigned long long *)option->data += value.u;
        break;
    case CLI_TYPE_U8:
        if (arg) {
            rc = parse_uint(arg, UINT8_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(uint8_t *)option->data += value.u;
        break;
    case CLI_TYPE_U16:
        if (arg) {
            rc = parse_uint(arg, UINT16_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(uint16_t *)option->data += value.u;
        break;
    case CLI_TYPE_U32:
        if (arg) {
            rc = parse_uint(arg, UINT32_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(uint32_t *)option->data += value.u;
        break;
    case CLI_TYPE_U64:
        if (arg) {
            rc = parse_uint(arg, UINT64_MAX, &value.u);
            if (rc)
                return rc;
        }
        *(uint64_t *)option->data += value.u;
        break;
    case CLI_TYPE_CHAR:
        if (arg) {
            rc = parse_int(arg, CHAR_MIN, CHAR_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(char *)option->data += value.s;
        break;
    case CLI_TYPE_SHORT:
        if (arg) {
            rc = parse_int(arg, SHRT_MIN, SHRT_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(short *)option->data += value.s;
        break;
    case CLI_TYPE_INT:
        if (arg) {
            rc = parse_int(arg, INT_MIN, INT_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(int *)option->data += value.s;
        break;
    case CLI_TYPE_LONG:
        if (arg) {
            rc = parse_int(arg, LONG_MIN, LONG_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(long *)option->data += value.s;
        break;
    case CLI_TYPE_LONGLONG:
        if (arg) {
            rc = parse_int(arg, LLONG_MIN, LLONG_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(long long *)option->data += value.s;
        break;
    case CLI_TYPE_I8:
        if (arg) {
            rc = parse_int(arg, INT8_MIN, INT8_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(int8_t *)option->data += value.s;
        break;
    case CLI_TYPE_I16:
        if (arg) {
            rc = parse_int(arg, INT16_MIN, INT16_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(int16_t *)option->data += value.s;
        break;
    case CLI_TYPE_I32:
        if (arg) {
            rc = parse_int(arg, INT32_MIN, INT32_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(int32_t *)option->data += value.s;
        break;
    case CLI_TYPE_I64:
        if (arg) {
            rc = parse_int(arg, INT64_MIN, INT64_MAX, &value.s);
            if (rc)
                return rc;
        }
        *(int64_t *)option->data += value.s;
        break;
    case CLI_TYPE_FLOAT:
        if (arg) {
            value.f = strtof(arg, NULL);
            if (errno == ERANGE)
                return EX_USAGE;
        }
        *(float *)option->data += 1;
        break;
    case CLI_TYPE_DOUBLE:
        if (arg) {
            value.d = strtod(arg, NULL);
            if (errno == ERANGE)
                return EX_USAGE;
        }
        *(double *)option->data += 1;
        break;
    case CLI_TYPE_LONGDOUBLE:
        if (arg) {
            value.ld = strtold(arg, NULL);
            if (errno == ERANGE)
                return EX_USAGE;
        }
        *(long double *)option->data += 1;
        break;
    case CLI_TYPE_STRING:
        return EINVAL;
    }

    return 0;
}

int
cli_parse(const struct cli * const cli, const int argc, char * const * const argv)
{
    char c;
    void *buf;
    size_t buf_sz;
    int rc = EX_OK;
    size_t optionc;
    char *shortopts, *ptr;
    const struct cli_option *option;
#ifndef CLI_NO_GETOPT_LONG
    struct option *longopts;
#endif

    if (!cli)
        return EX_DATAERR;

    if (!cli_program_name)
        cli_set_program_name(argv[0]);

    optionc = cli_get_num_options(cli);
    buf_sz = (3 * optionc + 3) * sizeof(*shortopts);
#ifndef CLI_NO_GETOPT_LONG
    buf_sz += (optionc + 1) * sizeof(*longopts);
#endif
    buf = calloc(1, buf_sz);
    if (!buf)
        return ENOMEM;

    shortopts = ptr = buf;
#ifndef CLI_NO_GETOPT_LONG
    longopts = (void *)(shortopts + (3 * optionc + 3));
#endif
    *(ptr++) = '+';
    *(ptr++) = ':';

    option = SLIST_FIRST(&cli->options);
    for (size_t i = 0; i < optionc; i++, option = SLIST_NEXT(option, entry)) {
#ifndef CLI_NO_GETOPT_LONG
        struct option *longopt;

        longopt = longopts + i;

        longopt->val = option->shrt;
        longopt->name = option->lng;
        longopt->flag = NULL;
#endif

        *(ptr++) = option->shrt;

        switch (option->argument) {
        case CLI_ARGUMENT_NONE:
            break;
        case CLI_ARGUMENT_REQUIRED:
            *(ptr++) = ':';
#ifndef CLI_NO_GETOPT_LONG
            longopt->has_arg = required_argument;
#endif
            break;
        case CLI_ARGUMENT_OPTIONAL:
            *(ptr++) = ':';
            *(ptr++) = ':';
#ifndef CLI_NO_GETOPT_LONG
            longopt->has_arg = optional_argument;
#endif
            break;
        }
    }

#ifndef CLI_NO_GETOPT_LONG
    while ((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
#else
    while ((c = getopt(argc, argv, shortopts)) != -1) {
#endif
        switch (c) {
        case ':':
            cli_error("Missing argument for option '-%c'", optopt);
            cli_action_help(cli, stderr);
            rc = EX_USAGE;
            goto out;
        case '?':
            cli_error("Invalid option '-%c'", optopt);
            cli_action_help(cli, stderr);
            rc = EX_USAGE;
            goto out;
        default:
            option = cli_get_option(cli, c);

            switch (option->action) {
            case CLI_ACTION_HELP:
                cli_action_help(cli, stdout);
                break;
            case CLI_ACTION_STORE:
                switch (option->argument) {
                case CLI_ARGUMENT_NONE:
                    rc = EINVAL;
                    goto out;
                case CLI_ARGUMENT_OPTIONAL:
                case CLI_ARGUMENT_REQUIRED:
                    if (!optarg) {
                        rc = EX_USAGE;
                        cli_action_help(cli, stderr);
                        goto out;
                    }
                    rc = cli_action_store(cli, option, optarg);
                }
                break;
            case CLI_ACTION_ACCUMULATE:
                rc = cli_action_accumulate(cli, option, optarg);
                break;
            }
        }
    }

    // Free memory as early as possible
    free(buf);
    buf = NULL;

    if (optind != argc) {
        const struct cli *subcommand;
        const char *arg = argv[optind];

        SLIST_FOREACH(subcommand, &cli->subcommands, entry) {
            int diff;

            diff = strcmp(subcommand->name, arg);
            if (diff == 0)
                break;
        }

        if (subcommand) {
            rc = cli_parse(subcommand, argc - optind, argv + optind);
        } else {
            cli_error("Unknown subcommand: %s", arg);
            cli_action_help(cli, stderr);
            rc = EX_USAGE;
        }
    } else {
        if (cli->callback)
            cli->callback(cli, cli->ctx);
    }

out:
    free(buf);

    return rc;
}
