package com.mongodb.m2s.testcommon;

import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.utils.Ssh;
import org.bson.Document;

/**
 * @Descreption 公共方法类
 * @Author
 * @CreateDate 2023/8/15
 */
public class CommLib extends M2STestBase {

    // 初始化目录
    public static void initDir( Ssh ssh, String path ) throws Exception {
        try {
            ssh.exec( "ls " + path );
        } catch ( Exception e ) {
            if ( e.getMessage().contains( "No such file or directory" ) ) {
                ssh.exec( "mkdir -p " + path );
            } else {
                throw e;
            }
        }
    }

    // 删除目录
    public static void rmDir( Ssh ssh, String path ) throws Exception {
        try {
            ssh.exec( "ls " + path );
            ssh.exec( "rm -rf " + path );
        } catch ( Exception e ) {
            if ( e.getMessage().contains( "No such file or directory" ) ) {
                return;
            } else {
                throw e;
            }
        }
    }

    // 判断集合是否存在
    public static boolean collectionExist( MongoDatabase database,
            String collectionName ) {
        for ( String name : database.listCollectionNames() ) {
            if ( name.equals( collectionName ) ) {
                return true;
            }
        }
        return false;
    }

    // 判断是否为分片集群
    public static boolean isSharded( MongoClient client ) {
        MongoDatabase db = client.getDatabase( "admin" );
        Document document = new Document( "isMaster", 1 );
        Document result = db.runCommand( document );
        if ( result.containsKey( "msg" )
                && result.getString( "msg" ).equals( "isdbgrid" ) ) {
            return true;
        } else {
            return false;
        }
    }

    // 判断是否为副本集集群
    public static boolean isReplicaSet( MongoClient client ) {
        MongoDatabase db = client.getDatabase( "admin" );
        Document document = new Document( "isMaster", 1 );
        Document result = db.runCommand( document );
        if ( result.containsKey( "msg" )
                && result.getString( "msg" ).equals( "isdbgrid" ) ) {
            return false;
        }
        if ( result.containsKey( "setName" ) ) {
            return true;
        } else {
            return false;
        }
    }

    // 判断是否为独立模式
    public static boolean isStandalone( MongoClient client ) {
        MongoDatabase db = client.getDatabase( "admin" );
        Document document = new Document( "isMaster", 1 );
        Document result = db.runCommand( document );
        if ( result.containsKey( "msg" )
                && result.getString( "msg" ).equals( "isdbgrid" ) ) {
            return false;
        }
        if ( result.getBoolean( "ismaster" )
                && !result.containsKey( "setName" ) ) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 启动sniffer
     *
     * @param ssh
     *            ssh连接
     * @throws Exception
     */
    public static void snifferStart( Ssh ssh ) throws Exception {
        String capturePath = toolRootPath + "m2s-sniffer/capture/*";
        ssh.exec( "rm -rf " + capturePath );

        String snifferCommand = snifferPath + " server-start -l "
                + snifferListenPort + " -m " + snifferAddr;
        ssh.exec( snifferCommand );
    }

    /**
     * 停止sniffer并分析
     *
     * @param ssh
     *            ssh连接
     * @throws Exception
     */
    public static void snifferStopAndAnalyze( Ssh ssh ) throws Exception {
        String snifferCommand = snifferPath + " server-stop";
        ssh.exec( snifferCommand );

        snifferCommand = snifferPath + " analyze" + " --output-path "
                + snifferOutputPath;
        ssh.exec( snifferCommand );
    }

    public static MongoClient getSnifferClient() {
        // sniffer对外服务地址
        String snifferUri = "mongodb://" + remoteHost + ":" + snifferListenPort;
        MongoClient snifferClient = MongoClients.create( snifferUri );
        return snifferClient;
    }

    /**
     * 收集集群信息
     * 
     * @param ssh
     *            ssh连接
     * @throws Exception
     */
    public static void collectCluster( Ssh ssh ) throws Exception {
        String collectCommand = collectorPath + " -t cluster -o "
                + collectorOutputPath + " -u " + mongodbUri;
        ssh.exec( collectCommand );
    }

    /**
     * 收集集合信息
     * 
     * @param ssh
     *            ssh连接
     * @param sample
     *            采样数量
     * @throws Exception
     */
    public static void collectCollection( Ssh ssh, int sample )
            throws Exception {
        String collectCommand = collectorPath + " -t collection -o "
                + collectorOutputPath + " -u " + mongodbUri + " -s " + sample;
        ssh.exec( collectCommand );
    }

    /**
     * 分析集群信息
     * 
     * @param ssh
     *            ssh连接
     * @param clusterJsonPath
     *            集群信息json文件路径
     * @throws Exception
     */
    public static void analyzeCluster( Ssh ssh, String clusterJsonPath )
            throws Exception {
        String analyzeCommand = analyzerPath + " --clusterjson "
                + clusterJsonPath + " -t json --sdbversion " + sdbVersion
                + " -o " + analyzerOutputPath;
        ssh.exec( analyzeCommand );
    }

    /**
     * 分析集合信息
     * 
     * @param ssh
     *            ssh连接
     * @param collectionJsonPath
     *            集合信息json文件路径
     * @throws Exception
     */
    public static void analyzeCollection( Ssh ssh, String collectionJsonPath )
            throws Exception {
        String analyzeCommand = analyzerPath + " --collectionjson "
                + collectionJsonPath + " -t json --sdbversion " + sdbVersion
                + " -o " + analyzerOutputPath;
        ssh.exec( analyzeCommand );
    }

    /**
     * 分析sniffer环境信息
     * 
     * @param ssh
     *            ssh连接
     * @throws Exception
     */
    public static void analyzeSnifferEnvJson( Ssh ssh ) throws Exception {
        String analyzeCommand = analyzerPath + " --snifferenvjson "
                + toolRootPath
                + "m2s-sniffer/capture/env.json -t json --sdbversion "
                + sdbVersion + " -o " + analyzerOutputPath;
        ssh.exec( analyzeCommand );
    }

    // 获取mongodb版本
    public static String getMongoDBVersion( MongoClient client ) {
        MongoDatabase db = client.getDatabase( "admin" );
        Document document = new Document( "buildInfo", 1 );
        Document result = db.runCommand( document );
        return result.getString( "version" );
    }

    // 比较版本号,0为相等，1为version1大于version2，-1为version1小于version2
    public static int compareVersion( String version1, String version2 ) {
        String[] version1Array = version1.split( "\\." );
        String[] version2Array = version2.split( "\\." );
        int length = Math.max( version1Array.length, version2Array.length );
        for ( int i = 0; i < length; i++ ) {
            int v1 = i < version1Array.length
                    ? Integer.parseInt( version1Array[ i ] )
                    : 0;
            int v2 = i < version2Array.length
                    ? Integer.parseInt( version2Array[ i ] )
                    : 0;
            if ( v1 > v2 ) {
                return 1;
            } else if ( v1 < v2 ) {
                return -1;
            }
        }
        return 0;
    }

}
