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
    meson_version: '>= 0.58.0'
)

fs = import('fs')
pkg = import('pkgconfig')

cc = meson.get_compiler('c')
python = find_program('python3')

if python.found()
    # Compute the relative path used by compiler invocations.
    relative_path = run_command(
        python,
        '-c',
        'import os; print("{}/".format(os.path.relpath("@0@", "@1@")))'.format(
            meson.global_source_root(),
            meson.global_build_root(),
        ),
        check: true
    ).stdout().strip()

    # Strip relative path prefixes from the code if possible, otherwise hide them.
    # The following snippet is inspired by the SwayWM Project under the MIT license.
    add_project_arguments(
        cc.get_supported_arguments(
            '-fmacro-prefix-map=@0@='.format(relative_path)
        ),
        language: 'c'
    )
endif

add_project_arguments(cc.get_supported_arguments(
        '-D_POSIX_C_SOURCE=200809L',
        '-Wdeclaration-after-statement',
        '-Wconversion'
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
