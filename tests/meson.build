# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: 2023 Tristan Partin <tristan@partin.io>

# Version which made TAP the default
glib_dep = dependency('glib-2.0', version: '>= 2.62', required: true)

test_env = environment({
    'G_TEST_SRCDIR': meson.current_source_dir(),
    'G_TEST_BUILDDIR': meson.current_build_dir()
})

tests = {
    'output-test': {
        'c_args': glib_dep.version().version_compare('< 2.76') ?
            cc.get_supported_arguments('-Wno-conversion') : []
    },
    'parser-test': {},
    'program-test': {},
}

foreach t, params : tests
    e = executable(
        t,
        '@0@.c'.format(t),
        c_args: params.get('c_args', []),
        dependencies: [
            libcli_dep,
            glib_dep,
        ]
    )

    test(t, e, env: test_env, protocol: 'tap')
endforeach
