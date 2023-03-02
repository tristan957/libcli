<!--
SPDX-License-Identifier: MIT

SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>
-->

# `libcli`

`libcli` is a featureful command line parser for C/C++ programs.

## Features

- Automatic help output generation
- Supports both POSIX [`getopt(3)`](https://linux.die.net/man/3/getopt) and GNU
  `getopt_long(3)`[^1]
- Recursive subcommands
- Supports optional arguments through GNU `optional_argument`

[^1]: If long options support is requested.
