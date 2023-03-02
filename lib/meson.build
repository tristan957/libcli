# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>

libmerr_dep = dependency(
    'libmerr',
    default_options: [
        'warning_level=0',
        'werror=false',
        'tests=false',
    ]
)

# Make private include files visibile to tests and examples
add_project_arguments('-I' + meson.current_source_dir(), language: 'c')

compile_args = []
if cc.get_define('optional_argument', prefix: '#include <getopt.h>') == ''
    if get_option('optional-arguments').enabled()
        error('C define optional_argument not found in header getopt.h')
    endif
    compile_args += '-DCLI_NO_OPTIONAL_ARGUMENT'
endif
if not cc.has_header_symbol('getopt.h', 'getopt_long', required: get_option('long-options'))
    compile_args += '-DCLI_NO_GETOPT_LONG'
endif

libcli = library(
    'cli',
    'output.c',
    'parser.c',
    'program.c',
    c_args: compile_args,
    include_directories: libcli_includes,
    dependencies: [libmerr_dep]
)

libcli_dep = declare_dependency(
    link_with: libcli,
    compile_args: compile_args,
    include_directories: libcli_includes,
    dependencies: [libmerr_dep]
)

meson.override_dependency(meson.project_name(), libcli_dep)

pkg.generate(
    libcli,
    name: 'libcli',
    description: 'Command line parsing library',
    extra_cflags: compile_args,
    requires: 'libmerr'
)
