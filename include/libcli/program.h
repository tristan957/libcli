/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#ifndef LIBCLI_PROGRAM_H
#define LIBCLI_PROGRAM_H

extern const char *cli_program_name;
extern const char *cli_program_name_short;

void
cli_set_program_name(const char *argv_0);

#endif
