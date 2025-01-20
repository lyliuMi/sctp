libsctp_core_src = files(
    'doubly_list.c',
    'log.c',
    'l_epoll.c',
    'l_fsm.c',
    'l_hash.c',
    'l_rbtree.c',
    'l_thread.c',
    'l_timer.c',
    'pollbox.c'
)

libpthread = dependency('threads', required : true)

libsctp_core_inc = include_directories('.')

libsctp_core = library('sctpcore',
    sources : libsctp_core_src,
    # version : libogslib_version,
    # c_args : '-DOGS_CORE_COMPILATION',
    include_directories : [libsctp_core_inc, libinc],
    dependencies : [libpthread],
    install : true,
    install_dir : libdir
)

libsctp_core_dep = declare_dependency(
    link_with : libsctp_core,
    include_directories : [libsctp_core_inc, libinc],
    dependencies : [libpthread]
)