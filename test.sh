#!/usr/bin/env bash

SELF=$(readlink -f $(dirname $0))
PATH=/bin:/usr/bin:/sbin:/usr/sbin
LIB=${1:-"$SELF/space-management.so"}
FUSE=${2:-"$SELF/fusefs"}
FAILED=0

trap 'umountFs' EXIT

function mkcd()
{
    mkdir -p $@ && cd $1
}
function allocExt4Fs()
{
    dd if=/dev/zero of=img bs=1M count=10
    mkfs -q -text4 img
    sudo losetup --find --show img
}
function mountFs()
{
    mkdir -p mnt
    if [ "$FUSE" = "loop" ]; then
        sudo mount $(allocExt4Fs) mnt
        sudo chown -R $USER:$USER mnt
    else
        EMPTY_FUSEFS_MAX=$(( (1<<20) * 20 )) $FUSE mnt
    fi
}
function umountFs()
{
    if [ "$FUSE" = "loop" ]; then
        sudo umount mnt >& /dev/null
    else
        fusermount -zu mnt
    fi
}
function writeFile()
{
    dd if=/dev/zero of=mnt/.test conv=notrunc oflag=append bs=1M count=$@ >& /dev/null
}
function writeFileWithManagement()
{
    LD_PRELOAD="$LIB" dd if=/dev/zero of=mnt/.test conv=notrunc oflag=append bs=1M count=$@ >& /dev/null &
    sleep 1
}
function writerPid()
{
    lsof -n mnt/.test | awk '/^dd / {print $2}'
}
function assert()
{
    echo Failed
    exit 6 # ABRT
}
function writerExist()
{
    pid=$(writerPid)
    test -n "$pid" || return 1
    grep -q $'^State:\tT (stopped)$' /proc/$pid/status || return 1 # linux only
    ps u $pid >& /dev/null || return 1
    echo $pid
}

function basicTest()
{
    mountFs
    writeFile 1 || return 1
    writeFile 20 && return 1 # must fail

    writeFileWithManagement 1
    kill -9 $(writerExist)
}
function noRetriesTest()
{
    SPACE_MANAGEMENT_RETRIES=0 basicTest && assert || true # must fail
}
function monitorTest()
{
    mountFs
    writeFile 1 || return 1

    SPACE_MANAGEMENT_MONITOR= SPACE_MANAGEMENT_MONITOR_MAX=1 writeFileWithManagement 1
    kill -9 $(writerExist)
}
function noMonitorTest()
{
    mountFs
    writeFile 1 || return 1

    writeFileWithManagement 1
    writerExist >/dev/null && assert || true # must fail
}

function runTest()
{
    local ret

    echo ========== $@ ==========
    $@ && ret=0 || ret=1
    umountFs || assert

    if [ $ret -eq 1 ]; then
        echo $'\t[FAILED]'
        let ++FAILED
    fi

    return $ret
}

function prepare()
{
    if [ ! "$FUSE" = "loop" ]; then
        mount -t proc proc /proc
    fi
}

function main()
{
    prepare
    mkcd test

    runTest basicTest
    runTest noRetriesTest
    runTest monitorTest
    runTest noMonitorTest

    if [ $FAILED -gt 0 ]; then
        echo "Failed: $FAILED (tests)"
        exit 1
    else
        echo Passed
        exit 0
    fi
}

main
