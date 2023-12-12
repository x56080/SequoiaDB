package com.mongodb.dds_ctl.common;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;
import org.yaml.snakeyaml.Yaml;
import java.util.List;

/**
 * @Descreption
 * @Author wangxingming
 * @CreateDate 2023/10/17
 * @UpdateUser wangxingming
 * @UpdateDate 2023/10/17
 */
public class CommLib extends CTLTestBase {

    // 基本参数，字段为null，配置文件将删除该字段
    public static basicEntity createBaseEntity( String destination, String path,
            boolean logAppend, String bindIp, String port, String dbPath,
            String replSetName, String clusterRole, String configDB ) {
        basicEntity basicEntity = new basicEntity();

        // 配置 SystemLog 属性
        basicEntity.SystemLog systemLog = new basicEntity.SystemLog();
        systemLog.setDestination( destination );
        systemLog.setPath( path );
        systemLog.setLogAppend( logAppend );
        basicEntity.setSystemLog( systemLog );

        // 配置 Net 属性
        basicEntity.Net net = new basicEntity.Net();
        net.setBindIp( bindIp );
        net.setPort( port );
        basicEntity.setNet( net );

        // 配置 Storage 属性
        if ( dbPath != null ) {
            basicEntity.Storage storage = new basicEntity.Storage();
            storage.setDbPath( dbPath );
            basicEntity.setStorage( storage );
        }

        // 配置 Replication 属性
        if ( replSetName != null ) {
            basicEntity.Replication replication = new basicEntity.Replication();
            replication.setReplSetName( replSetName );
            basicEntity.setReplication( replication );
        }

        // 配置 Sharding 属性
        if ( ( clusterRole != null || configDB != null ) ) {
            basicEntity.Sharding sharding = new basicEntity.Sharding();
            sharding.setClusterRole( clusterRole );
            sharding.setConfigDB( configDB );
            basicEntity.setSharding( sharding );
        }

        return basicEntity;
    }

    // 额外参数，字段为null，配置文件将删除该字段
    public static extraEntity createExtraEntity( Boolean fork,
            String pidFilePath ) {
        extraEntity extraEntity = new extraEntity();

        // 配置 processManagement 属性
        if ( ( fork != null || pidFilePath != null ) ) {
            extraEntity.ProcessManagement processManagement = new extraEntity.ProcessManagement();
            processManagement.setFork( fork );
            processManagement.setPidFilePath( pidFilePath );
            extraEntity.setProcessManagement( processManagement );
        }
        return extraEntity;
    }

    // 搭建三节点副本集集群
    public static void createReplicaSet( Ssh ssh, Boolean append )
            throws Exception {
        Yaml yaml = new Yaml();

        // 生成配置文件
        basicEntity basicEntity1 = CommLib.createBaseEntity( "file",
                dataBasePath + "10000/mongod.log", true, "0.0.0.0", "10000",
                dataBasePath + "10000", "rs", null, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity1 ) + "' > "
                + configFilePath + "10000.yaml" );

        basicEntity basicEntity2 = CommLib.createBaseEntity( "file",
                dataBasePath + "11000/mongod.log", true, "0.0.0.0", "11000",
                dataBasePath + "11000", "rs", null, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity2 ) + "' > "
                + configFilePath + "11000.yaml" );

        basicEntity basicEntity3 = CommLib.createBaseEntity( "file",
                dataBasePath + "12000/mongod.log", true, "0.0.0.0", "12000",
                dataBasePath + "12000", "rs", null, null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity3 ) + "' > "
                + configFilePath + "12000.yaml" );

        // 配置文件追加额外参数
        if ( append == true ) {
            extraEntity extraEntity1 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity1 ) + "' >> "
                    + configFilePath + "10000.yaml" );

            extraEntity extraEntity2 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity2 ) + "' >> "
                    + configFilePath + "11000.yaml" );

            extraEntity extraEntity3 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity3 ) + "' >> "
                    + configFilePath + "12000.yaml" );
        }

        // 将配置文件中不需要的参数去掉
        ssh.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );

        // sdb_dds_ctl工具创建节点
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "10000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "11000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "12000.yaml" );

        // sdb_dds_ctl工具启动节点
        ssh.exec( ctlPath + " start --all" );

        // 初始化副本集集群
        ssh.exec( mongoshPath
                + " --port 10000 --eval=\"rs.initiate({_id:'rs',version:1,members:[{_id:0,host:'"
                + remoteHost + ":10000',priority: 2},{_id:1,host:'" + remoteHost
                + ":11000'},{_id:3,host:'" + remoteHost + ":12000'}]})\"" );
        CommLib.checkCluster( 10000 );

        // 删除configFile目录下的配置文件
        ssh.exec( "rm -rf " + configFilePath + "*.yaml" );
    }

    // 搭建三节点分片集群
    public static void createSharding( Ssh ssh, Boolean append )
            throws Exception {
        Yaml yaml = new Yaml();

        // 生成配置文件
        basicEntity basicEntity1 = CommLib.createBaseEntity( "file",
                dataBasePath + "10000/mongod.log", true, "0.0.0.0", "10000",
                dataBasePath + "10000", "rs", "configsvr", null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity1 ) + "' > "
                + configFilePath + "10000.yaml" );

        basicEntity basicEntity2 = CommLib.createBaseEntity( "file",
                dataBasePath + "11000/mongod.log", true, "0.0.0.0", "11000",
                dataBasePath + "11000", "rs", "configsvr", null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity2 ) + "' > "
                + configFilePath + "11000.yaml" );

        basicEntity basicEntity3 = CommLib.createBaseEntity( "file",
                dataBasePath + "12000/mongod.log", true, "0.0.0.0", "12000",
                dataBasePath + "12000", "rs", "configsvr", null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity3 ) + "' > "
                + configFilePath + "12000.yaml" );

        basicEntity basicEntity4 = CommLib.createBaseEntity( "file",
                dataBasePath + "13000/mongod.log", true, "0.0.0.0", "13000",
                dataBasePath + "13000", "shard", "shardsvr", null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity4 ) + "' > "
                + configFilePath + "13000.yaml" );

        basicEntity basicEntity5 = CommLib.createBaseEntity( "file",
                dataBasePath + "14000/mongod.log", true, "0.0.0.0", "14000",
                dataBasePath + "14000", "shard", "shardsvr", null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity5 ) + "' > "
                + configFilePath + "14000.yaml" );

        basicEntity basicEntity6 = CommLib.createBaseEntity( "file",
                dataBasePath + "15000/mongod.log", true, "0.0.0.0", "15000",
                dataBasePath + "15000", "shard", "shardsvr", null );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity6 ) + "' > "
                + configFilePath + "15000.yaml" );

        basicEntity basicEntity7 = CommLib.createBaseEntity( "file",
                dataBasePath + "16000/mongos.log", true, "0.0.0.0", "16000",
                null, null, null,
                "rs/localhost:10000,localhost:11000,localhost:12000" );
        ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity7 ) + "' > "
                + configFilePath + "16000.yaml" );

        // 配置文件追加额外参数
        if ( append == true ) {
            extraEntity extraEntity1 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity1 ) + "' >> "
                    + configFilePath + "10000.yaml" );

            extraEntity extraEntity2 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity2 ) + "' >> "
                    + configFilePath + "11000.yaml" );

            extraEntity extraEntity3 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity3 ) + "' >> "
                    + configFilePath + "12000.yaml" );

            extraEntity extraEntity4 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity4 ) + "' >> "
                    + configFilePath + "13000.yaml" );

            extraEntity extraEntity5 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity5 ) + "' >> "
                    + configFilePath + "14000.yaml" );

            extraEntity extraEntity6 = CommLib.createExtraEntity( true, null );
            ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity6 ) + "' >> "
                    + configFilePath + "15000.yaml" );
        }

        // 将配置文件中不需要的参数去掉
        ssh.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );

        // sdb_dds_ctl工具创建节点
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "10000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "11000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "12000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "13000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "14000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "15000.yaml" );
        ssh.exec( ctlPath + " init --configfile " + configFilePath
                + "16000.yaml" );

        // sdb_dds_ctl工具启动节点
        ssh.exec( ctlPath + " start --all" );

        // 初始化分片集群配置节点
        ssh.exec( mongoshPath
                + " --port 10000 --eval=\"rs.initiate({_id:'rs',version:1,members:[{_id:0,host:'"
                + remoteHost + ":10000',priority: 2},{_id:1,host:'" + remoteHost
                + ":11000'},{_id:3,host:'" + remoteHost + ":12000'}]})\"" );
        // 初始化分片集群数据节点
        ssh.exec( mongoshPath
                + " --port 13000 --eval=\"rs.initiate({_id:'shard',version:1,members:[{_id:0,host:'"
                + remoteHost + ":13000',priority: 2},{_id:1,host:'" + remoteHost
                + ":14000'},{_id:3,host:'" + remoteHost + ":15000'}]})\"" );
        CommLib.checkCluster( 13000 );
        // 初始化分片集群路由节点
        ssh.exec( mongoshPath + " --port 16000 --eval=\"sh.addShard('shard/"
                + remoteHost + ":13000," + remoteHost + ":14000," + remoteHost
                + ":15000')\"" );

        // 删除configFile目录下的配置文件
        ssh.exec( "rm -rf " + configFilePath + "*.yaml" );
    }

    // 在多台机器上搭建三节点分片集群
    public static void createShardingOnMulServers( Ssh ssh1, Ssh ssh2 )
            throws Exception {
        Yaml yaml = new Yaml();

        // 生成配置文件
        basicEntity entity1 = CommLib.createBaseEntity( "file",
                dataBasePath + "10000/mongod.log", true, "0.0.0.0", "10000",
                dataBasePath + "10000", "rs", "configsvr", null );
        ssh1.exec( "echo '" + yaml.dumpAsMap( entity1 ) + "' > "
                + configFilePath + "10000.yaml" );

        basicEntity entity2 = CommLib.createBaseEntity( "file",
                dataBasePath + "11000/mongod.log", true, "0.0.0.0", "11000",
                dataBasePath + "11000", "rs", "configsvr", null );
        ssh1.exec( "echo '" + yaml.dumpAsMap( entity2 ) + "' > "
                + configFilePath + "11000.yaml" );

        basicEntity entity3 = CommLib.createBaseEntity( "file",
                dataBasePath + "12000/mongod.log", true, "0.0.0.0", "12000",
                dataBasePath + "12000", "rs", "configsvr", null );
        ssh2.exec( "echo '" + yaml.dumpAsMap( entity3 ) + "' > "
                + configFilePath + "12000.yaml" );

        basicEntity entity4 = CommLib.createBaseEntity( "file",
                dataBasePath + "13000/mongod.log", true, "0.0.0.0", "13000",
                dataBasePath + "13000", "shard", "shardsvr", null );
        ssh1.exec( "echo '" + yaml.dumpAsMap( entity4 ) + "' > "
                + configFilePath + "13000.yaml" );

        basicEntity entity5 = CommLib.createBaseEntity( "file",
                dataBasePath + "14000/mongod.log", true, "0.0.0.0", "14000",
                dataBasePath + "14000", "shard", "shardsvr", null );
        ssh1.exec( "echo '" + yaml.dumpAsMap( entity5 ) + "' > "
                + configFilePath + "14000.yaml" );

        basicEntity entity6 = CommLib.createBaseEntity( "file",
                dataBasePath + "15000/mongod.log", true, "0.0.0.0", "15000",
                dataBasePath + "15000", "shard", "shardsvr", null );
        ssh2.exec( "echo '" + yaml.dumpAsMap( entity6 ) + "' > "
                + configFilePath + "15000.yaml" );

        basicEntity entity7 = CommLib.createBaseEntity( "file",
                dataBasePath + "16000/mongos.log", true, "0.0.0.0", "16000",
                null, null, null,
                "rs/" + ssh1.getHost() + ":10000," + ssh1.getHost() + ":11000,"
                        + ssh2.getHost() + ":12000" );
        ssh1.exec( "echo '" + yaml.dumpAsMap( entity7 ) + "' > "
                + configFilePath + "16000.yaml" );

        // 将配置文件中不需要的参数去掉
        ssh1.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );
        ssh2.exec( "find " + configFilePath
                + " -type f -exec sed -i '/: null$/d' {} \\;" );

        // sdb_dds_ctl工具创建节点
        ssh1.exec( ctlPath + " init --configfile " + configFilePath
                + "10000.yaml" );
        ssh1.exec( ctlPath + " init --configfile " + configFilePath
                + "11000.yaml" );
        ssh2.exec( ctlPath + " init --configfile " + configFilePath
                + "12000.yaml" );
        ssh1.exec( ctlPath + " init --configfile " + configFilePath
                + "13000.yaml" );
        ssh1.exec( ctlPath + " init --configfile " + configFilePath
                + "14000.yaml" );
        ssh2.exec( ctlPath + " init --configfile " + configFilePath
                + "15000.yaml" );
        ssh1.exec( ctlPath + " init --configfile " + configFilePath
                + "16000.yaml" );

        // sdb_dds_ctl工具启动节点
        ssh1.exec( ctlPath + " start --all" );
        ssh2.exec( ctlPath + " start --all" );

        // 初始化分片集群
        ssh1.exec( mongoshPath
                + " --port 10000 --eval=\"rs.initiate({_id:'rs',version:1,members:[{_id:0,host:'"
                + remoteHost + ":10000'},{_id:1,host:'" + remoteHost
                + ":11000'},{_id:3,host:'" + ssh2.getHost() + ":12000'}]})\"" );
        ssh1.exec( mongoshPath
                + " --port 13000 --eval=\"rs.initiate({_id:'shard',version:1,members:[{_id:0,host:'"
                + remoteHost + ":13000'},{_id:1,host:'" + remoteHost
                + ":14000'},{_id:3,host:'" + ssh2.getHost() + ":15000'}]})\"" );
        CommLib.checkCluster( 13000 );
        ssh1.exec( mongoshPath + " --port 16000 --eval=\"sh.addShard('shard/"
                + remoteHost + ":13000," + remoteHost + ":14000,"
                + ssh2.getHost() + ":15000')\"" );

        // 删除configFile目录下的配置文件
        ssh1.exec( "rm -rf " + configFilePath + "*.yaml" );
        ssh2.exec( "rm -rf " + configFilePath + "*.yaml" );

    }

    public static void createNode( Ssh ssh, basicEntity basicEntity,
            String configFileName, Boolean append ) throws Exception {
        try {
            Yaml yaml = new Yaml();

            ssh.exec( "echo '" + yaml.dumpAsMap( basicEntity ) + "' > "
                    + configFilePath + configFileName );
            // 配置文件追加额外参数
            if ( append == true ) {
                extraEntity extraEntity = CommLib.createExtraEntity( true,
                        null );
                ssh.exec( "echo '" + yaml.dumpAsMap( extraEntity ) + "' >> "
                        + configFilePath + configFileName );
            }
            // 将配置文件中不需要的参数去掉
            ssh.exec( "find " + configFilePath
                    + " -type f -exec sed -i '/: null$/d' {} \\;" );
            ssh.exec( ctlPath + " init --configfile " + configFilePath
                    + configFileName );
            // sdb_dds_ctl工具启动节点
            ssh.exec( ctlPath + " start --all" );
        } finally {
            // 删除configFile目录下的配置文件
            ssh.exec( "rm -rf " + configFilePath + configFileName );
        }
    }

    // 连接dds执行基本数据操作
    public static void crudDDS( MongoClient ddsClient ) {
        // 获取数据库
        MongoDatabase database = ddsClient.getDatabase( "mydb" );
        // 获取集合
        MongoCollection< Document > collection = database
                .getCollection( "mycollection" );
        // 插入文档
        Document document = new Document( "name", "John" ).append( "age", 30 );
        collection.insertOne( document );
        // 查询文档
        Assert.assertEquals( 30, collection.find().first().get( "age" ) );
        // 更新文档
        Document filter = new Document( "name", "John" );
        Document update = new Document( "$set", new Document( "age", 31 ) );
        collection.updateOne( filter, update );
        Assert.assertEquals( 31, collection.find().first().get( "age" ) );
        // 删除文档
        collection.deleteOne( filter );
        Assert.assertEquals( 0, collection.countDocuments() );
        // 删除集合
        collection.drop();
    }

    // 删除dds节点
    public static void deleteNode( Ssh ssh ) throws Exception {
        ssh.exec( ctlPath + " stop --all --force" );
        ssh.exec( ctlPath + " remove --all-nodes" );
    }

    // 校验dds节点信息
    public static void checkNode( String listString, String subString,
            String checkString ) {
        // 将输出信息按行分割
        Integer columnIndex = null;
        String[] lines = listString.split( "\n" );
        // 遍历每行，查找包含subString的行
        for ( String line : lines ) {
            String[] columns = line.split( "\\s+" );
            if ( line.contains( subString ) ) {
                for ( int i = 0; i < columns.length; i++ ) {
                    if ( columns[ i ].equals( subString ) ) {
                        columnIndex = i;
                        break;
                    }
                }
                continue;
            }
            if ( columnIndex != null ) {
                String columnString = columns[ columnIndex ];
                Assert.assertEquals( checkString, columnString,
                        "节点的展示字段信息不正确" );
            } else {
                System.out.println( "没有找到包含 " + subString + " 的行" );
                break;
            }
        }
    }

    // 判断集群是否初始化成功
    public static void checkCluster( Integer port ) throws Exception {
        for ( int i = 0; i < 20; i++ ) {
            Thread.sleep( 3000 );
            try ( MongoClient ddsClient = MongoClients
                    .create( "mongodb://" + remoteHost + ":" + port )) {
                MongoDatabase database = ddsClient.getDatabase( "admin" ); // 使用admin数据库
                Document isMaster = database
                        .runCommand( new Document( "isMaster", 1 ) );
                boolean isPrimary = isMaster.getBoolean( "ismaster" );
                if ( isPrimary == true ) {
                    return; // 集群初始化成功，退出函数
                }
            }
        }
        throw new Exception( "集群初始化失败" );
    }

    // 判断是否移除分片成功
    public static void checkShardRemove( MongoClient ddsClient,
            String shardName ) throws Exception {
        for ( int attempts = 0; attempts < 60; attempts++ ) {
            String state = ddsClient.getDatabase( "admin" )
                    .runCommand( new Document( "removeShard", shardName ) )
                    .getString( "state" );
            if ( state.equals( "completed" ) ) {
                return; // 分片操作完成，退出函数
            }
            Thread.sleep( 1000 );
        }
        throw new Exception( "移除分片失败" );
    }

    // ctl list命令输出信息的类型
    public enum listType {
        pid, port, replset_name, cluster_role, type, start_time, status, dbpath
    }

    // 获取ctl list命令输出信息
    public static void getListInfo( String origin, List< String > listInfo,
            listType type ) throws Exception {
        String[] line = origin.split( "\n" );
        for ( int i = 0; i < line.length; i++ ) {
            if ( line[ i ].contains( "pid" ) ) {
                continue;
            }
            String[] info = line[ i ].split( "\\s+" );
            switch ( type ) {
            case pid:
                listInfo.add( info[ 0 ] );
                break;
            case port:
                listInfo.add( info[ 1 ] );
                break;
            case replset_name:
                listInfo.add( info[ 2 ] );
                break;
            case cluster_role:
                listInfo.add( info[ 3 ] );
                break;
            case type:
                listInfo.add( info[ 4 ] );
                break;
            case start_time:
                listInfo.add( info[ 5 ] );
                break;
            case status:
                listInfo.add( info[ 6 ] );
                break;
            case dbpath:
                listInfo.add( info[ 7 ] );
                break;
            default:
                break;
            }
        }
    }
}
