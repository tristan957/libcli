# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>

examples = [
    'reuse',
]

foreach e : examples
    executable(e, '@0@.c'.format(e), dependencies: libcli_dep)
endforeach
