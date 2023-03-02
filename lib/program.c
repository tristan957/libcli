/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#include "util.h"

#include <string.h>

const char *cli_program_name;
const char *cli_program_name_short;

void
cli_set_program_name(const char * const argv_0)
{
    const char *slash;

    if (!argv_0)
        return;

    cli_program_name = argv_0;

    slash = strrchr(argv_0, PATH_SEP);
    if (slash) {
        cli_program_name_short = slash + 1;
    } else {
        cli_program_name_short = argv_0;
    }
}
