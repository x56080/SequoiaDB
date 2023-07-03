#!/bin/bash

hostName=`hostname`
dbPath="MongoDB"
toolPath=`pwd`
mode="replset"


# common function
function display()
{
   echo "$0 --help | -h"
   echo "$0 [-dbpath path] [-toolpath path] [-mode mode]"
   echo ""
   echo " -dbpath path : 指定节点安装路径"
   echo " -toolpath path : 指定 MongoDB 工具路径"
   echo " -mode mode : 指定部署模式，可选值为 replset/sharded/standalone，默认为 replset"

   echo ""
   exit $1
}

function installStandalone() {
   echo "==================== start install MongoDB ===================="

   # create data dir
   mkdir -p "${dbPath}/log"
   mkdir -p "${dbPath}/27017"

   # deploy mongo standalone instance
   "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27017 --dbpath "${dbPath}/27017" --logpath "${dbPath}/log/standalone.log" --fork

   echo "==================== finish install MongoDB ===================="
}

function installReplSet()
{
    echo "==================== start install MongoDB ===================="

    # create data dir
    mkdir -p "${dbPath}/log"
    mkdir -p "${dbPath}/27017"
    mkdir -p "${dbPath}/27020"
    mkdir -p "${dbPath}/27021"

    # deploy mongo cluster rs1
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27017 --dbpath "${dbPath}/27017" --logpath "${dbPath}/log/rs1-1.log" --replSet rs1 --fork
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27020 --dbpath "${dbPath}/27020" --logpath "${dbPath}/log/rs1-2.log" --replSet rs1 --fork
    "${toolPath}/mongod" --bind_ip localhost,${hostName} --port 27021 --dbpath "${dbPath}/27021" --logpath "${dbPath}/log/rs1-3.log" --replSet rs1 --fork
    "${toolPath}/mongo" --port 27017 --eval "rs.initiate({_id: 'rs1', members: [{_id: 0, host: '${hostName}:27017'}, {_id: 1, host: '${hostName}:27020'}, {_id: 2, host: '${hostName}:27021'}]})"

    echo "==================== finish install MongoDB ===================="
}

function installSharded()
{
    echo "==================== start install MongoDB ===================="

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

function installMongoDB()
{
    # stop mongoDB
    pkill -9 mongo

    # remove old data
    rm -r "${dbPath}" 2>/dev/null

   if [ "${mode}" == "replset" ]; then
      installReplSet
   elif [ "${mode}" == "sharded" ]; then
      installSharded
   elif [ "${mode}" == "standalone" ]; then
      installStandalone
   else
      echo "mode must be replset or sharded"
      display 1
      exit 1
   fi
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
      -mode )             shift
                          mode=$1
                          ;;
      --help | -h )       display 0
                          ;;
      * )                 echo "Invalid argument: $1"
                          display 1
                          ;;
   esac
   shift
done

installMongoDB