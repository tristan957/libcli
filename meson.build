# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>

project(
    'libcli',
    'c',
    version: files('VERSION'),
    license: 'MIT',
    default_options: [
        'buildtype=debugoptimized',
        'c_std=c11',
        'warning_level=3',
    ],
    meson_version: '>= 0.57.0'
)

fs = import('fs')
pkg = import('pkgconfig')

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments(
        '-D_POSIX_C_SOURCE=200809L',
        '-Wdeclaration-after-statement'
    ),
    language: 'c'
)

subdir('include')
subdir('lib')
if get_option('examples')
    subdir('examples')
endif
if get_option('tests')
    subdir('tests')
endif
