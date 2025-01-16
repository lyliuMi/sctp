project('sctp', 'c',
    version : '1.0.0',
    meson_version : '>= 0.56.0'
)

cc = meson.get_compiler('c')
host_system = host_machine.system()
project_path = meson.project_source_root()

prefix = project_path + '/install/'
bindir = prefix + 'bin'
libdir = prefix + 'lib'
confdir = prefix + 'conf'

subdir('config')
subdir('lib')
subdir('src')

message(
    'project_path = ' + project_path + '\n',
    'prefix = ' + prefix + '\n',
    'bindir = ' + bindir + '\n',
    'libdir = ' + libdir + '\n',
    'confdir = ' + confdir + '\n',
)