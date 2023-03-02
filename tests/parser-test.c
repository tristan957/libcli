/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include <errno.h>

#include <glib.h>

#include <libcli/parser.h>

static void
test_add_option_duplicates(void)
{
    int rc;
    struct cli cli = { .name = "test" };
    struct cli_option option = { .shrt = 's', .action = CLI_ACTION_HELP };

    rc = cli_add_option(&cli, &option);
    g_assert_no_errno(rc);

    rc = cli_add_option(&cli, &option);
    g_assert_cmpint(rc, ==, ENOTUNIQ);

    option.shrt = 'k';
    rc = cli_add_option(&cli, &option);
    g_assert_cmpint(rc, ==, ENOTUNIQ);

    rc = cli_add_option(&cli, &option);
    g_assert_cmpint(rc, ==, ENOTUNIQ);
}

static void
test_add_option_invald_args(void)
{
    int rc;

    struct cli cli = { .name = "test" };
    struct cli_option option = { .shrt = 'h', .action = CLI_ACTION_HELP };

    rc = cli_add_option(&cli, NULL);
    g_assert_cmpint(rc, ==, EINVAL);

    rc = cli_add_option(NULL, &option);
    g_assert_cmpint(rc, ==, EINVAL);
}

int
main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/parser/add_option/duplicates", test_add_option_duplicates);
    g_test_add_func("/parser/add_option/invalid-args", test_add_option_invald_args);

    return g_test_run();
}
