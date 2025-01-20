# libserver_src = files(
#     'client.c'
# )

tun_sources = files('''
    main.c
    ../main.c
'''.split())

executable('tun',
    sources : tun_sources,
    include_directories : src_inc,
    dependencies : [libsctp_core_dep, sctp_dep, lib_sctp_tun_dep],
    install_dir : bindir,
    install : true
)
