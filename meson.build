project('isocore', 'c', version : '2.31',
                        meson_version : '>=0.46.0',
                        license : 'BSD3',
                        default_options : [ 'c_std=c11', 'b_ndebug=if-release' ])

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
    '-D_POSIX_C_SOURCE=200809L',
    '-D_DEFAULT_SOURCE'
]), language : 'c')

m_dep = cc.find_library('m', required : false)
threads_dep = dependency('threads')
zlib_dep = dependency('zlib')
bz2_dep = cc.find_library('bz2', required : true)
lzma_dep = dependency('liblzma', version: '>=5.1.1')
lz4_dep = dependency('liblz4')

incdir = include_directories('include')

isocore = both_libraries('isocore',
    sources : [
        'src/bgp.c',
        'src/bgpattribs.c',
        'src/bgpparams.c',
        'src/bits.c',
        'src/cache.c',
        'src/dumppacket.c',
        'src/endian.c',
        'src/filtercompiler.c',
        'src/filterdump.c',
        'src/filterintrin.c',
        'src/filterpacket.c',
        'src/hexdump.c',
        'src/io.c',
        'src/json.c',
        'src/log.c',
        'src/mrt.c',
        'src/netaddr.c',
        'src/parse.c',
        'src/patriciatrie.c',
        'src/pool.c',
        'src/progutil.c',
        'src/sockets.c',
        'src/threading.c',
        'src/strutil.c',
        'src/u128_t.c',
        'src/vt100.c'
    ],
    include_directories : incdir,
    dependencies : [ m_dep, threads_dep, zlib_dep, bz2_dep, lz4_dep, lzma_dep ],
    install : true
)
install_subdir('include', install_dir : get_option('includedir'), strip_directory : true)

pkg = import('pkgconfig')
pkg.generate(
    name : meson.project_name(),
    version : meson.project_version(),
    libraries : isocore,
    description : 'The Isolario project core library.'
)

isocore_dep = declare_dependency(
    include_directories : incdir,
    link_with : isocore
)

if get_option('build-tests')
	cunit_dep = dependency('cunit')
	testincdir = include_directories('test')

	bgp_test = executable('bgp_test',
		sources : [
			'test/bgp/attribs.c',
			'test/bgp/main.c',
			'test/bgp/open.c',
			'test/bgp/update.c'
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
			'test/core/dumppacket_t.c',
			'test/core/hexdump_t.c',
			'test/core/io_t.c',
			'test/core/json_t.c',
			'test/core/log_t.c',
			'test/core/netaddr_t.c',
			'test/core/patriciatrie_t.c',
			'test/core/strutil_t.c',
			'test/core/u128_t.c'
		],
		dependencies : [
			isocore_dep,
			cunit_dep
		],
		include_directories : testincdir
	)
	test('core', core_test)

	filter_test = executable('filter_test',
		sources : [
			'test/filter/main.c',
			'test/filter/mrt_filter_t.c'
		],
		dependencies : [
			isocore_dep,
			cunit_dep
		],
		include_directories : testincdir
	)
endif

if get_option('build-examples')

	simple_parse_example = executable('simple_parse_example',
		sources : [ 'example/simple_parse_example.c' ],
		dependencies : [ isocore_dep ]
	)

	simple_calc_example = executable('simple_calc_example',
		sources : [ 'example/simple_calc_example.c' ],
		dependencies : [ isocore_dep, m_dep ]
	)

endif

if get_option('build-benchmarks')
	cbench_dep = dependency('cbench', static : true, fallback : [ 'cbench', 'cbench_dep' ])
	benchincdir = include_directories('bench')

	core_bench = executable('core_bench',
		sources : [
			'bench/core/main.c',
			'bench/core/strutil_b.c',
			'bench/core/patriciatrie_b.c',
			'bench/core/netaddr_b.c'
		],
		dependencies : [
			isocore_dep,
			cbench_dep
		],
		include_directories : benchincdir
	)
	benchmark('core', core_bench)

	bgp_bench = executable('bgp_bench',
		sources : [
			'bench/bgp/main.c',
			'bench/bgp/update_b.c'
		],
		dependencies : [
			isocore_dep,
			cbench_dep
		],
		include_directories : benchincdir
	)
	benchmark('bgp', bgp_bench)

endif
