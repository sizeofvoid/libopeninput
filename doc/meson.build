prg_install = find_program('install')

doxygen = find_program('doxygen', required : false)
if not doxygen.found()
	error('Program "doxygen" not found or not executable. Try building with -Ddocumentation=false')
endif
dot = find_program('dot', required : false)
if not dot.found()
	error('Program "dot" not found or not executable. Try building with -Ddocumentation=false')
endif

doxygen_version_cmd = run_command(doxygen.path(), '--version')
if doxygen_version_cmd.returncode() != 0
	error('Command "doxygen --version" failed.')
endif
doxygen_version = doxygen_version_cmd.stdout()
if doxygen_version.version_compare('< 1.8.3')
	error('doxygen needs to be at least version 1.8.3 (have @0@)'.format(doxygen_version))
endif
grep = find_program('grep')
dot_version_cmd = run_command(dot.path(), '-V')
if dot_version_cmd.returncode() != 0
	error('Command "dot -V" failed.')
endif
# dot -V output is (to stderr):
# 	dot - graphviz version 2.38.0 (20140413.2041)
dot_version = dot_version_cmd.stderr().split(' ')[4]
if dot_version.version_compare('< 2.26')
	error('Graphviz dot needs to be at least version 2.26 (have @0@)'.format(dot_version))
endif

readme = vcs_tag(command : ['git', 'log', '-1', '--format=%h'],
		 fallback : 'unknown',
		 input : '../README.md',
		 output : 'README.md',
		 replace_string: '__GIT_VERSION__')

src_doxygen = files(
	# source files
	'../src/libinput.h',
	# written docs
	'absolute-axes.dox',
	'absolute-coordinate-ranges.dox',
	'architecture.dox',
	'building.dox',
	'button_debouncing.dox',
	'clickpad-softbuttons.dox',
	'contributing.dox',
	'device-configuration-via-udev.dox',
	'device-quirks.dox',
	'faqs.dox',
	'gestures.dox',
	'middle-button-emulation.dox',
	'normalization-of-relative-motion.dox',
	'palm-detection.dox',
	'page-hierarchy.dox',
	'pointer-acceleration.dox',
	'reporting-bugs.dox',
	'scrolling.dox',
	'seats.dox',
	'switches.dox',
	't440-support.dox',
	'tablet-support.dox',
	'tapping.dox',
	'test-suite.dox',
	'timestamps.dox',
	'tools.dox',
	'touchpad-jumping-cursors.dox',
	'touchpad-pressure.dox',
	'touchpad-jitter.dox',
	'touchpads.dox',
	'trackpoints.dox',
	'what-is-libinput.dox',
	# dot drawings
	'dot/seats-sketch.gv',
	'dot/seats-sketch-libinput.gv',
	'dot/libinput-stack-wayland.gv',
	'dot/libinput-stack-xorg.gv',
	'dot/libinput-stack-gnome.gv',
	'dot/evemu.gv',
	# svgs
	'svg/button-debouncing-wave-diagram.svg',
	'svg/button-scrolling.svg',
	'svg/clickfinger.svg',
	'svg/clickfinger-distance.svg',
	'svg/edge-scrolling.svg',
	'svg/gesture-2fg-ambiguity.svg',
	'svg/palm-detection.svg',
	'svg/pinch-gestures.svg',
	'svg/pinch-gestures-softbuttons.svg',
	'svg/ptraccel-linear.svg',
	'svg/ptraccel-low-dpi.svg',
	'svg/ptraccel-touchpad.svg',
	'svg/ptraccel-trackpoint.svg',
	'svg/software-buttons.svg',
	'svg/swipe-gestures.svg',
	'svg/tablet-axes.svg',
	'svg/tablet-cintiq24hd-modes.svg',
	'svg/tablet-interfaces.svg',
	'svg/tablet-intuos-modes.svg',
	'svg/tablet-left-handed.svg',
	'svg/tablet-out-of-bounds.svg',
	'svg/tablet.svg',
	'svg/tap-n-drag.svg',
	'svg/thumb-detection.svg',
	'svg/top-software-buttons.svg',
	'svg/touchscreen-gestures.svg',
	'svg/trackpoint-delta-illustration.svg',
	'svg/twofinger-scrolling.svg',
	# style files
	'style/header.html',
	'style/footer.html',
	'style/customdoxygen.css',
	'style/bootstrap.css',
	'style/libinputdoxygen.css',
)

doxyfiles = custom_target('doxyfiles',
			  input : src_doxygen,
			  output : '.',
			  command : [prg_install, '-t', '@OUTDIR@', '@INPUT@'],
			  build_by_default: true)

doc_config = configuration_data()
doc_config.set('PACKAGE_NAME', meson.project_name())
doc_config.set('PACKAGE_VERSION', meson.project_version())
doc_config.set('builddir', meson.current_build_dir())

doxyfile = configure_file(input : 'libinput.doxygen.in',
			  output : 'libinput.doxygen',
			  configuration : doc_config,
			  install : false)

custom_target('doxygen',
	      input : [ doxyfile, readme ] + src_doxygen,
	      output : [ 'Documentation' ],
	      command : [ doxygen, doxyfile ],
	      install : false,
	      depends: [doxyfiles, readme],
	      build_by_default : true)
