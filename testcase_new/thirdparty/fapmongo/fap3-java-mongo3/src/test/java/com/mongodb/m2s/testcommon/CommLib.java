package com.mongodb.m2s.testcommon;

import com.mongodb.MongoClient;
import com.mongodb.client.MongoDatabase;
import com.mongodb.utils.Ssh;
import org.bson.Document;

/**
 * @Descreption
 * @Author
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/15
 * @UpdateRemark
 * @Version
 */
public class CommLib {

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

    public static boolean collectionExist( MongoDatabase database,
            String collectionName ) {
        for ( String name : database.listCollectionNames() ) {
            if ( name.equals( collectionName ) ) {
                return true;
            }
        }
        return false;
    }

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

    public static boolean isReplicaSet( MongoClient client ) {
        MongoDatabase db = client.getDatabase( "admin" );
        Document document = new Document( "isMaster", 1 );
        Document result = db.runCommand( document );
        if ( result.containsKey( "setName" ) ) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean isStandalone( MongoClient client ) {
        MongoDatabase db = client.getDatabase( "admin" );
        Document document = new Document( "isMaster", 1 );
        Document result = db.runCommand( document );
        if ( result.getBoolean( "ismaster" ) ) {
            return true;
        } else {
            return false;
        }
    }

    public static void execCmd( String cmd ) {
        try {
            Process process = Runtime.getRuntime().exec( cmd );
            process.waitFor();
        } catch ( Exception e ) {
            e.printStackTrace();
        }
    }

    public static void collectCluster( Ssh ssh, String collectorUri,
            String collectorOutputPath ) throws Exception {
        String collectCommand = collectorUri + " -t cluster -o "
                + collectorOutputPath;
        ssh.exec( collectCommand );
    }

    public static void collectCollection( Ssh ssh, String CollectorUri,
            String collectorOutputPath ) throws Exception {
        String collectCommand = CollectorUri + " -t collection -o "
                + collectorOutputPath;
        ssh.exec( collectCommand );
    }

    public static void analyzeCluster( Ssh ssh, String clusterJsonPath,
            String analyzerUri, String analyzerOutputPath, String sdbversion )
            throws Exception {
        String analyzeCommand = analyzerUri + " --clusterjson "
                + clusterJsonPath + " -t json --sdbversion " + sdbversion
                + " -o " + analyzerOutputPath;
        ssh.exec( analyzeCommand );
    }

    public static void analyzeCollection( Ssh ssh, String collectionJsonPath,
            String analyzerUri, String analyzerOutputPath, String sdbversion )
            throws Exception {
        String analyzeCommand = analyzerUri + " --collectionjson "
                + collectionJsonPath + " -t json --sdbversion " + sdbversion
                + " -o " + analyzerOutputPath;
        ssh.exec( analyzeCommand );
    }

}
