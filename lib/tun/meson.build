lib_sctp_tun_src = files(
    'tun.c'
)


lib_sctp_tun_inc = include_directories('.')

lib_sctp_tun = library('sctptun',
    sources : lib_sctp_tun_src,
    include_directories : [lib_sctp_tun_inc, libinc],
    dependencies : [libsctp_core_dep],
    install : true,
    install_dir : libdir
)

lib_sctp_tun_dep = declare_dependency(
    link_with : lib_sctp_tun,
    include_directories : [lib_sctp_tun_inc, libinc],
    dependencies : [libsctp_core_dep]
)