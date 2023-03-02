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

#include <merr.h>

#include <libcli/output.h>
#include <libcli/parser.h>
#include <libcli/program.h>

#define TAB "  "

merr_t
cli_add_argument(struct cli * const cli, struct cli_argument * const argument)
{
    struct cli_argument *a;
    struct cli_argument *prev;

    if (!cli || !argument)
        return merr(EINVAL);

    if (SLIST_EMPTY(&cli->arguments)) {
        SLIST_INSERT_HEAD(&cli->arguments, argument, entry);

        return 0;
    }

    prev = SLIST_FIRST(&cli->arguments);
    SLIST_FOREACH(a, &cli->arguments, entry) {
        if (a == argument)
            return merr(ENOTUNIQ);

        if (strcmp(a->name, argument->name) > 0)
            break;

        /* We want to continue looping through the rest of the options to verify
         * other preconditions.
         */
        if (!prev)
            prev = a;
    }

    SLIST_INSERT_AFTER(prev, argument, entry);

    return 0;
}

merr_t
cli_add_arguments(struct cli *cli, const size_t argumentc, struct cli_argument * const argumentv)
{
    if (!cli || !argumentv)
        return merr(EINVAL);

    for (size_t i = 0; i < argumentc; i++) {
        merr_t err;

        err = cli_add_argument(cli, argumentv + i);
        if (err)
            return err;
    }

    return 0;
}

merr_t
cli_add_option(struct cli * const cli, struct cli_option * const option)
{
    struct cli_option *o;
    struct cli_option *prev;

    if (!cli || !option)
        return merr(EINVAL);

    if (SLIST_EMPTY(&cli->options)) {
        SLIST_INSERT_HEAD(&cli->options, option, entry);

        return 0;
    }

    prev = SLIST_FIRST(&cli->options);
    SLIST_FOREACH(o, &cli->options, entry) {
        if (o == option || o->shrt == option->shrt)
            return merr(ENOTUNIQ);

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

merr_t
cli_add_options(struct cli *cli, const size_t optionc, struct cli_option * const optionv)
{
    if (!cli || !optionv)
        return merr(EINVAL);

    for (size_t i = 0; i < optionc; i++) {
        merr_t err;

        err = cli_add_option(cli, optionv + i);
        if (err)
            return err;
    }

    return 0;
}

merr_t
cli_add_subcommand(struct cli *cli, struct cli * const subcommand)
{
    struct cli *c;
    struct cli *prev;

    if (!cli || !subcommand)
        return merr(EINVAL);

    if (cli == subcommand)
        return merr(EINVAL);

    if (SLIST_EMPTY(&cli->subcommands)) {
        SLIST_INSERT_HEAD(&cli->subcommands, subcommand, entry);

        return 0;
    }

    prev = SLIST_FIRST(&cli->subcommands);
    SLIST_FOREACH(c, &cli->subcommands, entry) {
        int rc;

        rc = strcmp(c->name, subcommand->name);
        if (c == subcommand || rc == 0)
            return merr(ENOTUNIQ);

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

merr_t
cli_add_subcommands(struct cli *cli, size_t subcommandc, struct cli * const subcommandv)
{
    if (!cli || !subcommandv)
        return merr(EINVAL);

    for (size_t i = 0; i < subcommandc; i++) {
        merr_t err;

        err = cli_add_subcommand(cli, subcommandv + i);
        if (err)
            return err;
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
cli_get_option(const struct cli * const cli, const int c)
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
cli_action_help(const struct cli * const cli, int * const exit_code, FILE * const output)
{
    assert(cli);
    assert(output);

    fprintf(output, "Usage: %s", cli_program_name);
    if (!SLIST_EMPTY(&cli->options))
        fputs(" [OPTIONS]...", output);
    if (!SLIST_EMPTY(&cli->arguments)) {
        const struct cli_argument *a;

        SLIST_FOREACH(a, &cli->arguments, entry)
            fprintf(output, " %s", a->name);
    }
    fputc('\n', output);

    if (cli->description)
        fprintf(output, "\n%s\n", cli->description);

    if (!SLIST_EMPTY(&cli->arguments)) {
        size_t max_width = 0;
        const struct cli_argument *a;

        fputs("\nArguments:\n", output);

        SLIST_FOREACH(a, &cli->arguments, entry) {
            size_t width = strlen(a->name);

            if (width > max_width)
                max_width = width;
        }

        SLIST_FOREACH(a, &cli->arguments, entry) {
            fprintf(output, TAB "%-*s", (int)max_width, a->name);
            if (a->description)
                fprintf(output, TAB "%s", a->description);
            fputc('\n', output);
        }
    }

    if (!SLIST_EMPTY(&cli->options)) {
        size_t max_width = 0;
        const struct cli_option *o;

        fputs("\nOptions:\n", output);

        SLIST_FOREACH(o, &cli->options, entry) {
            size_t width = 0;

#ifndef CLI_NO_GETOPT_LONG
            if (o->lng)
                width += strlen(o->lng);
#endif

            switch (o->argument) {
            case CLI_HAS_ARG_NONE:
                break;
            case CLI_HAS_ARG_REQUIRED:
                width += 3;
                break;
#ifndef CLI_NO_OPTIONAL_ARGUMENT
            case CLI_HAS_ARG_OPTIONAL:
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
            case CLI_HAS_ARG_NONE:
                break;
            case CLI_HAS_ARG_REQUIRED:
                arg_str = " arg";
                break;
#ifndef CLI_NO_OPTIONAL_ARGUMENT
            case CLI_HAS_ARG_OPTIONAL:
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
        const struct cli *c;
        size_t max_width = 0;

        fputs("\nSubcommands:\n", output);

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

    if (output == stderr && exit_code)
        *exit_code = EX_USAGE;
}

static bool
parse_bool(const char * const arg, int * const exit_code, bool * const value)
{
    assert(arg);
    assert(value);

    if (strcmp(arg, "true") == 0) {
        *value = true;
    } else if (strcmp(arg, "false") == 0) {
        *value = false;
    } else {
        if (exit_code)
            *exit_code = EX_USAGE;
        return false;
    }

    return true;
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

static bool
parse_uint(
    const char * const arg,
    int * const exit_code,
    const unsigned long long max,
    unsigned long long * const value)
{
    int base;

    assert(arg);
    assert(value);

    base = parse_base(arg);

    errno = 0;
    *value = strtoull(arg + (base == 10 ? 0 : 2), NULL, base);
    if (errno == ERANGE || *value > max) {
        if (exit_code)
            *exit_code = EX_USAGE;
        return false;
    }

    return true;
}

static bool
parse_int(
    const char * const arg,
    int * const exit_code,
    const long long min,
    const long long max,
    long long * const value)
{
    int base;

    assert(arg);
    assert(value);

    base = parse_base(arg);

    errno = 0;
    *value = strtoll(arg + (base == 10 ? 0 : 2), NULL, base);
    if (errno == ERANGE || *value < min || *value > max) {
        if (exit_code)
            *exit_code = EX_USAGE;
        return false;
    }

    return true;
}

static void
cli_action_store(
    const struct cli * const cli,
    int * const exit_code,
    const struct cli_option * const option,
    const char *arg)
{
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
            if (!parse_bool(arg, exit_code, option->data))
                return;
        } else {
            *(bool *)option->data = true;
        }
        break;
    case CLI_TYPE_UCHAR:
        if (!parse_uint(arg, exit_code, UCHAR_MAX, &value.u))
            return;
        *(unsigned char *)option->data = (unsigned char)value.u;
        break;
    case CLI_TYPE_USHORT:
        if (!parse_uint(arg, exit_code, USHRT_MAX, &value.u))
            return;
        *(unsigned short *)option->data = (unsigned short)value.u;
        break;
    case CLI_TYPE_UINT:
        if (!parse_uint(arg, exit_code, UINT_MAX, &value.u))
            return;
        *(unsigned int *)option->data = (unsigned int)value.u;
        break;
    case CLI_TYPE_ULONG:
        if (!parse_uint(arg, exit_code, ULONG_MAX, &value.u))
            return;
        *(unsigned long *)option->data = value.u;
        break;
    case CLI_TYPE_ULONGLONG:
        if (!parse_uint(arg, exit_code, ULLONG_MAX, &value.u))
            return;
        *(unsigned long long *)option->data = value.u;
        break;
    case CLI_TYPE_U8:
        if (!parse_uint(arg, exit_code, UINT8_MAX, &value.u))
            return;
        *(uint8_t *)option->data = (uint8_t)value.u;
        break;
    case CLI_TYPE_U16:
        if (!parse_uint(arg, exit_code, UINT16_MAX, &value.u))
            return;
        *(uint16_t *)option->data = (uint16_t)value.u;
        break;
    case CLI_TYPE_U32:
        if (!parse_uint(arg, exit_code, UINT32_MAX, &value.u))
            return;
        *(uint32_t *)option->data = (uint32_t)value.u;
        break;
    case CLI_TYPE_U64:
        if (!parse_uint(arg, exit_code, UINT64_MAX, &value.u))
            return;
        *(uint64_t *)option->data = value.u;
        break;
    case CLI_TYPE_CHAR:
        if (!parse_int(arg, exit_code, CHAR_MIN, CHAR_MAX, &value.s))
            return;
        *(char *)option->data = (char)value.s;
        break;
    case CLI_TYPE_SHORT:
        if (!parse_int(arg, exit_code, SHRT_MIN, SHRT_MAX, &value.s))
            return;
        *(short *)option->data = (short)value.s;
        break;
    case CLI_TYPE_INT:
        if (!parse_int(arg, exit_code, INT_MIN, INT_MAX, &value.s))
            return;
        *(int *)option->data = (int)value.s;
        break;
    case CLI_TYPE_LONG:
        if (!parse_int(arg, exit_code, LONG_MIN, LONG_MAX, &value.s))
            return;
        *(long *)option->data = value.s;
        break;
    case CLI_TYPE_LONGLONG:
        if (!parse_int(arg, exit_code, LLONG_MIN, LLONG_MAX, &value.s))
            return;
        *(long long *)option->data = value.s;
        break;
    case CLI_TYPE_I8:
        if (!parse_int(arg, exit_code, INT8_MIN, INT8_MAX, &value.s))
            return;
        *(int8_t *)option->data = (int8_t)value.s;
        break;
    case CLI_TYPE_I16:
        if (!parse_int(arg, exit_code, INT16_MIN, INT16_MAX, &value.s))
            return;
        *(int16_t *)option->data = (int16_t)value.s;
        break;
    case CLI_TYPE_I32:
        if (!parse_int(arg, exit_code, INT32_MIN, INT32_MAX, &value.s))
            return;
        *(int32_t *)option->data = (int32_t)value.s;
        break;
    case CLI_TYPE_I64:
        if (!parse_int(arg, exit_code, INT64_MIN, INT64_MAX, &value.s))
            return;
        *(int64_t *)option->data = (int64_t)value.s;
        break;
    case CLI_TYPE_FLOAT:
        value.f = strtof(arg, NULL);
        if (errno == ERANGE) {
            if (exit_code)
                *exit_code = EX_USAGE;
            return;
        }
        *(float *)option->data = value.f;
        break;
    case CLI_TYPE_DOUBLE:
        value.d = strtod(arg, NULL);
        if (errno == ERANGE) {
            if (exit_code)
                *exit_code = EX_USAGE;
            return;
        }
        *(double *)option->data = value.d;
        break;
    case CLI_TYPE_LONGDOUBLE:
        value.ld = strtold(arg, NULL);
        if (errno == ERANGE) {
            if (exit_code)
                *exit_code = EX_USAGE;
            return;
        }
        *(long double *)option->data = value.ld;
        break;
    case CLI_TYPE_STRING:
        *(const char **)option->data = arg;
        break;
    }
}

static merr_t
cli_action_accumulate(
    const struct cli * const cli,
    int * const exit_code,
    const struct cli_option * const option,
    const char * const arg)
{
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
            if (!parse_uint(arg, exit_code, UCHAR_MAX, &value.u))
                return 0;
        }
        *(unsigned char *)option->data += (unsigned char)value.u;
        break;
    case CLI_TYPE_USHORT:
        if (arg) {
            if (!parse_uint(arg, exit_code, USHRT_MAX, &value.u))
                return 0;
        }
        *(unsigned short *)option->data += (unsigned short)value.u;
        break;
    case CLI_TYPE_UINT:
        if (arg) {
            if (!parse_uint(arg, exit_code, UINT_MAX, &value.u))
                return 0;
        }
        *(unsigned int *)option->data += (unsigned int)value.u;
        break;
    case CLI_TYPE_ULONG:
        if (arg) {
            if (!parse_uint(arg, exit_code, ULONG_MAX, &value.u))
                return 0;
        }
        *(unsigned long *)option->data += value.u;
        break;
    case CLI_TYPE_ULONGLONG:
        if (arg) {
            if (!parse_uint(arg, exit_code, ULLONG_MAX, &value.u))
                return 0;
        }
        *(unsigned long long *)option->data += value.u;
        break;
    case CLI_TYPE_U8:
        if (arg) {
            if (!parse_uint(arg, exit_code, UINT8_MAX, &value.u))
                return 0;
        }
        *(uint8_t *)option->data += (uint8_t)value.u;
        break;
    case CLI_TYPE_U16:
        if (arg) {
            if (!parse_uint(arg, exit_code, UINT16_MAX, &value.u))
                return 0;
        }
        *(uint16_t *)option->data += (uint16_t)value.u;
        break;
    case CLI_TYPE_U32:
        if (arg) {
            if (!parse_uint(arg, exit_code, UINT32_MAX, &value.u))
                return 0;
        }
        *(uint32_t *)option->data += (uint32_t)value.u;
        break;
    case CLI_TYPE_U64:
        if (arg) {
            if (!parse_uint(arg, exit_code, UINT64_MAX, &value.u))
                return 0;
        }
        *(uint64_t *)option->data += (uint64_t)value.u;
        break;
    case CLI_TYPE_CHAR:
        if (arg) {
            if (!parse_int(arg, exit_code, CHAR_MIN, CHAR_MAX, &value.s))
                return 0;
        }
        *(char *)option->data += (char)value.s;
        break;
    case CLI_TYPE_SHORT:
        if (arg) {
            if (!parse_int(arg, exit_code, SHRT_MIN, SHRT_MAX, &value.s))
                return 0;
        }
        *(short *)option->data += (short)value.s;
        break;
    case CLI_TYPE_INT:
        if (arg) {
            if (!parse_int(arg, exit_code, INT_MIN, INT_MAX, &value.s))
                return 0;
        }
        *(int *)option->data += (int)value.s;
        break;
    case CLI_TYPE_LONG:
        if (arg) {
            if (!parse_int(arg, exit_code, LONG_MIN, LONG_MAX, &value.s))
                return 0;
        }
        *(long *)option->data += value.s;
        break;
    case CLI_TYPE_LONGLONG:
        if (arg) {
            if (!parse_int(arg, exit_code, LLONG_MIN, LLONG_MAX, &value.s))
                return 0;
        }
        *(long long *)option->data += value.s;
        break;
    case CLI_TYPE_I8:
        if (arg) {
            if (!parse_int(arg, exit_code, INT8_MIN, INT8_MAX, &value.s))
                return 0;
        }
        *(int8_t *)option->data += (int8_t)value.s;
        break;
    case CLI_TYPE_I16:
        if (arg) {
            if (!parse_int(arg, exit_code, INT16_MIN, INT16_MAX, &value.s))
                return 0;
        }
        *(int16_t *)option->data += (int16_t)value.s;
        break;
    case CLI_TYPE_I32:
        if (arg) {
            if (!parse_int(arg, exit_code, INT32_MIN, INT32_MAX, &value.s))
                return 0;
        }
        *(int32_t *)option->data += (int32_t)value.s;
        break;
    case CLI_TYPE_I64:
        if (arg) {
            if (!parse_int(arg, exit_code, INT64_MIN, INT64_MAX, &value.s))
                return 0;
        }
        *(int64_t *)option->data += (int64_t)value.s;
        break;
    case CLI_TYPE_FLOAT:
        if (arg) {
            value.f = strtof(arg, NULL);
            if (errno == ERANGE) {
                if (exit_code)
                    *exit_code = EX_USAGE;
                return 0;
            }
        }
        *(float *)option->data += 1;
        break;
    case CLI_TYPE_DOUBLE:
        if (arg) {
            value.d = strtod(arg, NULL);
            if (errno == ERANGE) {
                if (exit_code)
                    *exit_code = EX_USAGE;
                return 0;
            }
        }
        *(double *)option->data += 1;
        break;
    case CLI_TYPE_LONGDOUBLE:
        if (arg) {
            value.ld = strtold(arg, NULL);
            if (errno == ERANGE) {
                if (exit_code)
                    *exit_code = EX_USAGE;
                return 0;
            }
        }
        *(long double *)option->data += 1;
        break;
    case CLI_TYPE_STRING:
        return merr(EINVAL);
    }

    return 0;
}

merr_t
cli_parse(const struct cli * const cli, const int argc, char * const * const argv, int *exit_code)
{
    int c;
    void *buf;
    size_t buf_sz;
    merr_t err = 0;
    size_t optionc;
    char *shortopts, *ptr;
    const struct cli_option *option;
#ifndef CLI_NO_GETOPT_LONG
    struct option *longopts;
#endif

    if (!cli)
        return merr(EINVAL);

    if (!cli_program_name)
        cli_set_program_name(argv[0]);

    optionc = cli_get_num_options(cli);
    buf_sz = (3 * optionc + 3) * sizeof(*shortopts);
#ifndef CLI_NO_GETOPT_LONG
    buf_sz += (optionc + 1) * sizeof(*longopts);
#endif
    buf = calloc(1, buf_sz);
    if (!buf)
        return merr(ENOMEM);

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
        case CLI_HAS_ARG_NONE:
            break;
        case CLI_HAS_ARG_REQUIRED:
            *(ptr++) = ':';
#ifndef CLI_NO_GETOPT_LONG
            longopt->has_arg = required_argument;
#endif
            break;
        case CLI_HAS_ARG_OPTIONAL:
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
            cli_error("Missing argument for option: '-%c'", optopt);
            cli_action_help(cli, exit_code, stderr);
            goto out;
        case '?':
            cli_error("Invalid option: '-%c'", optopt);
            cli_action_help(cli, exit_code, stderr);
            goto out;
        default:
            option = cli_get_option(cli, c);

            switch (option->action) {
            case CLI_ACTION_HELP:
                cli_action_help(cli, exit_code, stdout);
                break;
            case CLI_ACTION_STORE:
                switch (option->argument) {
                case CLI_HAS_ARG_NONE:
                    err = merr(EINVAL);
                    goto out;
                case CLI_HAS_ARG_OPTIONAL:
                case CLI_HAS_ARG_REQUIRED:
                    if (!optarg) {
                        cli_action_help(cli, exit_code, stderr);
                        goto out;
                    }
                    cli_action_store(cli, exit_code, option, optarg);
                }
                break;
            case CLI_ACTION_ACCUMULATE:
                cli_action_accumulate(cli, exit_code, option, optarg);
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
            err = cli_parse(subcommand, argc - optind, argv + optind, exit_code);
        } else {
            cli_error("Unknown subcommand: %s", arg);
            cli_action_help(cli, exit_code, stderr);
            goto out;
        }
    }

    if (cli->callback) {
        cli->callback(cli, exit_code, cli->ctx);
    } else {
        if (exit_code)
            *exit_code = 0;
    }

out:
    free(buf);

    return err;
}
