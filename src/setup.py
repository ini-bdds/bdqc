from distutils.core import setup,Extension

setup(name="bdqc",
	version="0.30",
	description="Framework for QC of \"Big Data\"",
	long_description="""\
	Framework for QC of \"Big Data\"
	""",
	author="Roger Kramer",
	author_email="rkramer@systemsbiology.org",
	url="www.systemsbiology.org",
#	py_modules=['scan'],
	packages=[
		'bdqc',
		'bdqc.builtin',
		'bdqc.builtin.extrinsic',
		'bdqc.builtin.filetype',
		'bdqc.builtin.tabular',
		'bdqc.builtin.dummy' ],
#	package_dir=
	package_data={'bdqc':['report-d3.js','report.css']},
	ext_package='bdqc.builtin', # ...scopes all extensions
	ext_modules=[
		Extension('compiled',
			['c/fopenx.c',
				'c/module.c',

				'c/tabular/utf8.c',
				'c/tabular/murmur3.c',
				'c/tabular/format.c',
				'c/tabular/strbag.c',
				'c/tabular/sspp.c',
				'c/tabular/scan.c',
				'c/tabular/analyze.c',
				'c/tabular/csv.c',
				'c/tabular/json.c',

				'c/stats/bounds.c',
				'c/stats/central.c',
				'c/stats/density.c',
				'c/stats/fft.c',
				'c/stats/gaussian.c',
				'c/stats/interp.c',
				'c/stats/mcnaive.c',
				'c/stats/quantile.c',
				'c/stats/quicksel.c'
			 ],
			 extra_compile_args=['-std=c99'],
			 define_macros=[("_POSIX_C_SOURCE","200809L"),])],
	data_files=[('.bdqc',['data/plugins.txt',]),]
	)

