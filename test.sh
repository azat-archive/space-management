#!/usr/bin/env bash

SELF=$(readlink -f $(dirname $0))
PATH=/bin:/usr/bin:/sbin:/usr/sbin
LIB="$SELF/space-management.so"

trap 'umountFs' EXIT

function mkcd()
{
    mkdir -p $@ && cd $1
}
function allocFs()
{
    fallocate -l10M img
    mkfs -q -text4 img
    sudo losetup --find --show img
}
function mountFs()
{
    mkdir -p mnt
    sudo mount $(allocFs) mnt
    sudo chown -R $USER:$USER mnt
}
function umountFs()
{
    sudo umount mnt >& /dev/null
}
function writeFile()
{
    dd if=/dev/zero of=mnt/.test conv=notrunc oflag=append bs=1M count=$@ >& /dev/null
}
function assert()
{
    echo Failed
    exit 6 # ABRT
}

function main()
{
    mkcd test

    mountFs
    writeFile 1 || assert
    writeFile 20 && assert # must fail

    LD_PRELOAD="$LIB" writeFile 1 >& /dev/null &
    local pid=$(lsof -n mnt/.test | awk '/^dd / {print $2}')
    test -n "$pid" || assert
    grep -q $'^State:\tT (stopped)$' /proc/$pid/status || assert # linux only
    ps u $pid >& /dev/null || assert
    kill -9 $pid
}

main
echo Passed
