# libserver_src = files(
#     'client.c'
# )

server_sources = files('''
    server.c
    ../main.c
'''.split())

executable('server',
    sources : server_sources,
    include_directories : src_inc,
    dependencies : [libsctp_core_dep, sctp_dep],
    install_dir : bindir,
    install : true
)
