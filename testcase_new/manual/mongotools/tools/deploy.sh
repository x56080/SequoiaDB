#!/bin/bash

hostName=`hostname`
dbPath="MongoDB"
toolPath=`pwd`

# common function
function display()
{
   echo "$0 --help | -h"
   echo "$0 [-dbpath path] [-toolpath path]"
   echo ""
   echo " -dbpath path : 指定节点安装路径"
   echo " -toolpath path : 指定 MongoDB 工具路径"

   echo ""
   exit $1
}

function installMongoDB()
{
    echo "==================== start install MongoDB ===================="

    # stop mongoDB
    pkill -9 mongo

    # remove old data
    rm -r "${dbPath}" 2>/dev/null

    # create data dir
    mkdir -p "${dbPath}/log"
    mkdir -p "${dbPath}/27020"
    mkdir -p "${dbPath}/27030"
    mkdir -p "${dbPath}/27031"
    mkdir -p "${dbPath}/27032"
    mkdir -p "${dbPath}/27100"

    # deploy mongo cluster rs0
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27020 --dbpath "${dbPath}/27020" --logpath "${dbPath}/log/rs0-1.log" --shardsvr --replSet rs0 --fork
    "${toolPath}/mongo" --port 27020 --eval "rs.initiate({_id: 'rs0', members: [{_id: 0, host: '${hostName}:27020'}]})"
    
    # deploy mongo cluster rs1
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27030 --dbpath "${dbPath}/27030" --logpath "${dbPath}/log/rs1-1.log" --shardsvr --replSet rs1 --fork
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27031 --dbpath "${dbPath}/27031" --logpath "${dbPath}/log/rs1-2.log" --shardsvr --replSet rs1 --fork
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27032 --dbpath "${dbPath}/27032" --logpath "${dbPath}/log/rs1-3.log" --shardsvr --replSet rs1 --fork
    "${toolPath}/mongo" --port 27030 --eval "rs.initiate({_id: 'rs1', members: [{_id: 0, host: '${hostName}:27030'}, {_id: 1, host: '${hostName}:27031'}, {_id: 2, host: '${hostName}:27032'}]})"

    # deploy mongo cluster conf
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27100 --dbpath "${dbPath}/27100" --logpath "${dbPath}/log/conf-1.log" --configsvr --replSet conf --fork
    sleep 2
    "${toolPath}/mongo" --port 27100 --eval "rs.initiate({_id: 'conf', members: [{_id: 0, host: '${hostName}:27100'}]})"
    sleep 2

    # deploy mongo cluster mongos
    "${toolPath}/mongos" --bind_ip localhost,${hostName} --port 27017 --configdb conf/${hostName}:27100 --logpath "${dbPath}/log/mongos.log" --fork
    sleep 2
    "${toolPath}/mongo" --port 27017 --eval "sh.addShard('rs0/${hostName}:27020')"
    "${toolPath}/mongo" --port 27017 --eval "sh.addShard('rs1/${hostName}:27030,${hostName}:27031,${hostName}:27032')"
    sleep 2
    echo "==================== finish install MongoDB ===================="
}

# read param
while [ "$1" != "" ]; do
   case $1 in
      -dbpath )           shift
                          dbPath=$(readlink -f $1)
                          ;;
      -toolpath )         shift
                          toolPath=$(readlink -f $1)
                          ;;
      --help | -h )       display 0
                          ;;
   esac
   shift
done

installMongoDB