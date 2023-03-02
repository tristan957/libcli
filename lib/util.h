/* SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
 */

#define NELEM(arr) (sizeof(arr) / sizeof(arr[0]))

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif
