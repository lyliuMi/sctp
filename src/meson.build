exe_src = files(
    'main.c',
    'log_test.c'
)

executable('sctp_test', 
    sources : exe_src,
    # include_directories: [_sctp_inc],
    dependencies : [libsctp_core_dep],
    install : true,
    install_dir : bindir
)