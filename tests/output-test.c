/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include "util.h"

#include <stdio.h>

#include <glib.h>

#include <libcli/output.h>
#include <libcli/program.h>

static void
test_cli_print_table(void)
{
    static const char *headers[] = { "ENGLISH", "SPANISH" };
    static const char *values[] = { "one", "uno", "two", "dos", "three", "tres" };
    static const enum cli_justify justify[] = { CLI_JUSTIFY_LEFT, CLI_JUSTIFY_RIGHT };
    static const bool enabled[] = { true, false };
    static const char one[] = "ENGLISH  SPANISH\n"
                              "one      uno\n"
                              "two      dos\n"
                              "three    tres\n";
    static const char two[] = "ENGLISH  SPANISH\n"
                              "one          uno\n"
                              "two          dos\n"
                              "three       tres\n";
    static const char three[] = "ENGLISH\n"
                                "one    \n"
                                "two    \n"
                                "three  \n";

    char *buf;
    FILE *stream;
    size_t buf_sz;
    int printed = 0;
    size_t offset = 0;

    stream = open_memstream(&buf, &buf_sz);

    printed = cli_print_table(stream, 3, NELEM(headers), headers, values, NULL, NULL);
    fflush(stream);
    g_assert_cmpint(printed, ==, sizeof(one) - 1);
    g_assert_cmpmem(buf + offset, buf_sz - offset, one, sizeof(one) - 1);
    offset += (size_t)printed;

    printed = cli_print_table(stream, 3, NELEM(headers), headers, values, justify, NULL);
    fflush(stream);
    g_assert_cmpint(printed, ==, sizeof(two) - 1);
    g_assert_cmpmem(buf + offset, buf_sz - offset, two, sizeof(two) - 1);
    offset += (size_t)printed;

    printed = cli_print_table(stream, 3, NELEM(headers), headers, values, justify, enabled);
    fflush(stream);
    g_assert_cmpint(printed, ==, sizeof(three) - 1);
    g_assert_cmpmem(buf + offset, buf_sz - offset, three, sizeof(three) - 1);
    offset += (size_t)printed;

    fclose(stream);
    free(buf);
}

int
main(int argc, char *argv[])
{
    cli_set_program_name(argv[0]);

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/cli/print_table", test_cli_print_table);

    return g_test_run();
}
