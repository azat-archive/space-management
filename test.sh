#!/usr/bin/env bash

SELF=$(readlink -f $(dirname $0))
PATH=/bin:/usr/bin:/sbin:/usr/sbin
LIB=${1:-"$SELF/space-management.so"}
FUSE=${2:-"$SELF/fusefs"}

trap 'umountFs' EXIT

function mkcd()
{
    mkdir -p $@ && cd $1
}
function mountFs()
{
    mkdir -p mnt
    $FUSE mnt
}
function umountFs()
{
    fusermount -zu mnt
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
}

function basicTest()
{
    mountFs
    writeFile 1 || return 1
    writeFile 20 && return 1 # must fail

    writeFileWithManagement 1
    writerExist && kill -9 $pid
}
function noRetriesTest()
{
    SPACE_MANAGEMENT_RETRIES=0 basicTest && assert # must fail
}
function monitorTest()
{
    mountFs
    writeFile 1 || return 1

    SPACE_MANAGEMENT_MONITOR= SPACE_MANAGEMENT_MONITOR_MAX=1 writeFileWithManagement 1
    writerExist
    kill -9 $pid
}
function noMonitorTest()
{
    mountFs
    writeFile 1 || return 1

    writeFileWithManagement 1
    writerExist && assert # must fail
}

function runTest()
{
    local ret

    echo ========== $@ ==========
    $@ && ret=0 || ret=1
    umountFs

    return $ret
}

function main()
{
    mkcd test

    runTest basicTest
    runTest noRetriesTest
    runTest monitorTest
    runTest noMonitorTest
}

main
echo Passed
