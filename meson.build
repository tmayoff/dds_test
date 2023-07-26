project(
    'dds_test',
    'cpp',
    version: '0.1',
    default_options: ['warning_level=3', 'cpp_std=c++20'],
)

opendds = dependency('OpenDDS')

sources = []

idl_compiler = find_program('idl_compiler.py')
idl_out_args = [
    '@BASENAME@C.h',
    '@BASENAME@TypeSupportImpl.h',
    '@BASENAME@TypeSupportImpl.cpp',
    '@BASENAME@TypeSupportC.cpp',
]
idl_cmd_args = [
    idl_compiler,
    '-o',
    '@OUTDIR@',
    '-s',
    '@CURRENT_SOURCE_DIR@',
    '-I' + meson.current_source_dir(),
    '@INPUT@',
]

subdir('IDL')

exe = executable(
    'dds_test',
    ['dds_test.cpp', 'ReaderListener.cpp'] + sources + idl_sources,
    dependencies: [opendds],
    include_directories: include_directories('.', 'IDL'),
    install: true,
)

test('basic', exe)
