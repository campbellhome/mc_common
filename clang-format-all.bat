wsl clang-format -i include/*.h include/mc_callstack/*.h include/md5_rfc1321/*.h include/uuid_rfc4122/*.h src/*.c src/mc_callstack/*.c* src/mc_preproc/*.* src/md5_rfc1321/*.c src/uuid_rfc4122/*.c
pushd submodules\bbclient
call clang-format-all.bat
popd