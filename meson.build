project('isocore', 'c', version : '2.31',
                    meson_version : '>=0.45.0',
                    license : 'BSD3',
                    default_options : [ 'c_std=c11' ])

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments([
    '-Wall',
    '-pedantic',
    '-Wextra',
    '-Wno-missing-braces',
    '-Wno-missing-field-initializers'
]), language : 'c')

m_dep = cc.find_library('m', required : false)
cunit_dep = dependency('cunit')

incdir = include_directories('include')

add_project_arguments(
    '-D_LARGEFILE_SOURCE',
    '-D_LARGEFILE64_SOURCE',
    '-D_FILE_OFFSET_BITS=64',
    '-D_POSIX_C_SOURCE=200809L',
    language : 'c'
)

isocore = library('isocore',
    sources : [
        'src/backtrace.c',
        'src/bgp.c',
        'src/bgpattribs.c',
        'src/bgpparams.c',
        'src/bgpprefix.c',
        'src/cache.c',
        'src/endian.c',
        'src/hexdump.c',
        'src/log.c',
        'src/parse.c',
        'src/strutil.c',
        'src/uint128_t.c'
    ],
    include_directories : incdir,
    dependencies : [ m_dep, cunit_dep ],
    install : true
)
install_subdir('include', install_dir : 'include')

isocore_dep = declare_dependency(
    include_directories : incdir,
    link_with : isocore
)

testincdir = include_directories('test')

bgp_test = executable('bgp_test',
    sources : [
        'test/bgp/attribs.c',
        'test/bgp/main.c',
        'test/bgp/open.c'
    ],
    dependencies : [
        isocore_dep,
        cunit_dep
    ],
    include_directories : testincdir
)
test('bgp', bgp_test)

core_test = executable('core_test',
    sources : [
        'test/core/hexdump.c',
        'test/core/main.c',
        'test/core/uint128.c'
    ],
    dependencies : [
        isocore_dep,
        cunit_dep
    ],
    include_directories : testincdir
)
test('core', core_test)

strutil_test = executable('strutil_test',
    sources : [
        'test/strutil/main.c',
        'test/strutil/splitjoin.c',
        'test/strutil/trim.c'
    ],
    dependencies : [
        isocore_dep,
        cunit_dep
    ],
    include_directories : testincdir
)
test('strutil', strutil_test)

log_test = executable('log_test',
    sources : [
        'test/log/main.c',
        'test/log/log.c',
    ],
    dependencies : [
        isocore_dep,
        cunit_dep
    ],
    include_directories : testincdir
)
test('log', log_test)

