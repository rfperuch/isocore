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
    '-Wno-missing-field-initializers',
    '-D_LARGEFILE_SOURCE',
    '-D_LARGEFILE64_SOURCE',
    '-D_FILE_OFFSET_BITS=64',
    '-D_POSIX_C_SOURCE=200809L'
]), language : 'c')

m_dep = cc.find_library('m', required : false)
threads_dep = dependency('threads')
bz2_dep = cc.find_library('bz2', required : true)
zlib_dep = dependency('zlib')
lz4_dep = cc.find_library('lz4', required : true)
cunit_dep = dependency('cunit')
# TODO cbench_dep = dependency('cbench' fallback : [ 'cbench', 'cbench_dep' ])

incdir = include_directories('include')

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
        'src/io.c',
        'src/log.c',
        'src/netaddr.c',
        'src/parse.c',
        'src/patriciatrie.c',
        'src/pool.c',
        'src/sockets.c',
        'src/threading.c',
        'src/strutil.c',
        'src/uint128_t.c',
        'src/vt100.c'
    ],
    include_directories : incdir,
    dependencies : [ m_dep, threads_dep, zlib_dep, bz2_dep, lz4_dep ],
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
        'test/core/main.c',
        'test/core/hexdump.c',
        'test/core/io.c',
        'test/core/log.c',
        'test/core/netaddr.c',
        'test/core/patriciatrie.c',
        'test/core/strutil.c',
        'test/core/uint128.c'
    ],
    dependencies : [
        isocore_dep,
        cunit_dep
    ],
    include_directories : testincdir
)
test('core', core_test)

