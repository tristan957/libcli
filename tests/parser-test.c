/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include <errno.h>
#include <stdio.h>

#include <glib.h>
#include <merr.h>

#include <libcli/parser.h>

static void
test_add_option_duplicates(void)
{
    merr_t err;
    struct cli cli = { .name = "test" };
    struct cli_option option = { .shrt = 's', .action = CLI_ACTION_HELP };

    err = cli_add_option(&cli, &option);
    g_assert_no_errno(merr_errno(err));

    err = cli_add_option(&cli, &option);
    g_assert_cmpint(merr_errno(err), ==, ENOTUNIQ);

    option.shrt = 'k';
    err = cli_add_option(&cli, &option);
    g_assert_cmpint(merr_errno(err), ==, ENOTUNIQ);

    err = cli_add_option(&cli, &option);
    g_assert_cmpint(merr_errno(err), ==, ENOTUNIQ);
}

static void
test_add_option_invald_args(void)
{
    merr_t err;

    struct cli cli = { .name = "test" };
    struct cli_option option = { .shrt = 'h', .action = CLI_ACTION_HELP };

    err = cli_add_option(&cli, NULL);
    g_assert_cmpint(merr_errno(err), ==, EINVAL);

    err = cli_add_option(NULL, &option);
    g_assert_cmpint(merr_errno(err), ==, EINVAL);
}

int
main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/parser/add_option/duplicates", test_add_option_duplicates);
    g_test_add_func("/parser/add_option/invalid-args", test_add_option_invald_args);

    return g_test_run();
}
