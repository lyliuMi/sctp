client_sources = files('''
    client.c
    ../main.c
'''.split())

executable('client',
    sources : client_sources,
    include_directories : src_inc,
    dependencies : [libsctp_core_dep, sctp_dep],
    install_dir : bindir,
    install : true
)