#!/bin/sh
# check environment variables

set -e

common_objpfx=$1; shift
elf_objpfx=$1; shift
rtld_installed_name=$1; shift

library_path=${common_objpfx}:${common_objpfx}nptl
tst_mutex8="${elf_objpfx}${rtld_installed_name} --library-path ${library_path} \
	  ${common_objpfx}/nptl/tst-mutex8"

export PTHREAD_MUTEX=adaptive 
tst_mutex8
export PTHREAD_MUTEX=elision
tst_mutex8
export PTHREAD_MUTEX=none
tst_mutex8

# XXX rwlock with elision 2 
