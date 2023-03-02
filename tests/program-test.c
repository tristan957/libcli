/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include "util.h"

#include <glib.h>

#include <libcli/program.h>

static void
test_cli_set_program_name(const void *data)
{
    const char *slash, *base;

    g_assert_cmpstr(data, ==, cli_program_name);

    slash = strrchr(data, PATH_SEP);
    if (slash) {
        base = slash + 1;
    } else {
        base = data;
    }

    g_assert_cmpstr(base, ==, cli_program_name_short);
}

int
main(int argc, char *argv[])
{
    cli_set_program_name(argv[0]);

    g_test_init(&argc, &argv, NULL);

    g_test_add_data_func("/cli/set_program_name", argv[0], test_cli_set_program_name);

    return g_test_run();
}
