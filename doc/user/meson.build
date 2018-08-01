# Sphinx build
sphinx = find_program('sphinx-build-3', 'sphinx-build', required : false)
if not sphinx.found()
	error('Program "sphinx-build" not found or not executable. Try building with -Ddocumentation=false')
endif

sphinx_config = configuration_data()
sphinx_config.set('PROJECT_NAME', meson.project_name())
sphinx_config.set('PROJECT_VERSION', meson.project_version())
sphinx_config.set('BUILDDIR', meson.current_build_dir())

git_version_page = vcs_tag(command : ['git', 'log', '-1', '--format=%H'],
			   fallback : 'unknown',
			   input : 'git_version.py.in',
			   output : 'git_version.py',
			   replace_string: '__GIT_VERSION__')

sphinx_conf_py = configure_file(input : 'conf.py.in',
				output : 'conf.py',
				configuration : sphinx_config,
				install : false)

src_extra = [
	# dot drawings
	'dot/seats-sketch.gv',
	'dot/seats-sketch-libinput.gv',
	'dot/libinput-stack-wayland.gv',
	'dot/libinput-stack-xorg.gv',
	'dot/libinput-stack-gnome.gv',
	'dot/evemu.gv',
	'dot/libinput-record.gv',
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
	'svg/software-buttons-conditions.svg',
	'svg/software-buttons-thumbpress.svg',
	'svg/software-buttons-visualized.svg',
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
]

src_rst = files(
	'absolute-axes.rst',
	'absolute-coordinate-ranges.rst',
	'architecture.rst',
	'building.rst',
	'button_debouncing.rst',
	'clickpad-softbuttons.rst',
	'contributing.rst',
	'device-configuration-via-udev.rst',
	'device-quirks.rst',
	'faqs.rst',
	'gestures.rst',
	'index.rst',
	'middle-button-emulation.rst',
	'normalization-of-relative-motion.rst',
	'palm-detection.rst',
	'pointer-acceleration.rst',
	'reporting-bugs.rst',
	'scrolling.rst',
	'seats.rst',
	'switches.rst',
	't440-support.rst',
	'tablet-support.rst',
	'tapping.rst',
	'test-suite.rst',
	'timestamps.rst',
	'tools.rst',
	'touchpad-jumping-cursors.rst',
	'touchpad-pressure.rst',
	'touchpad-jitter.rst',
	'touchpads.rst',
	'trackpoints.rst',
	'what-is-libinput.rst',
	'features.rst',
	'development.rst',
	'troubleshooting.rst',
	'configuration.rst',
)

src_sphinx = custom_target('sphinx-sources',
			   input : [src_rst, src_extra],
			   output : '.',
			   command : [prg_install, '-t', '@OUTDIR@', '@INPUT@'],
			   build_by_default: true)


# drop '-a' once we are happy with all this
# do not use -j, it breaks on Ubuntu
sphinx_output_dir = 'Documentation'
custom_target('sphinx',
	      input : [ sphinx_conf_py, git_version_page ] + src_rst + src_extra,
	      output : [ sphinx_output_dir ],
	      command : [ sphinx, '-q', '-b', 'html', '-a',
			  meson.current_build_dir(), sphinx_output_dir],
	      depends: [ src_sphinx ],
	      build_by_default : true)
