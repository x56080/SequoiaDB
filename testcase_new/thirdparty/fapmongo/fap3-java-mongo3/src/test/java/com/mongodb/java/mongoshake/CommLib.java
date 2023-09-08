package com.mongodb.java.mongoshake;

import com.jcraft.jsch.JSchException;
import com.mongodb.client.*;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;

import java.util.Arrays;
import java.util.List;

/**
 * @Descreption
 * @Author wangxingming
 * @CreateDate 2023/8/2
 * @UpdateUser wangxingming
 * @UpdateDate 2023/8/2
 */
public class CommLib {
    static String host = "192.168.17.117";
    static String username = "root";
    static String password = "sequoiadb";
    static int port = 22;
    static String toolPath = "/data/sequoiashake/";
    static String MongoDBHost = "192.168.17.117";
    static String SequoiaDBHost = "192.168.17.117";
    static int MongoDBPort = 27018;
    static int SequoiaDBPort = 11810;
    static int SequoiaDBFapPort = SequoiaDBPort + 7;
    static String incrMonitor = "curl -s http://127.0.0.1:9100/repl | python -m json.tool";
    static String confFileFull = "collector_full.conf";
    static String confFileIncr = "collector_incr.conf";
    static String confFileAll = "collector_all.conf";

    // 创建SSH连接，返回ssh对象
    public static Ssh createSsh() throws JSchException {
        Ssh ssh = new Ssh( host, username, password, port );
        return ssh;
    }

    // 构造数据
    public static void genDataToMongoDB( String configFile, int workerNum,
            Ssh ssh ) throws Exception {
        String confDir = toolPath + "config/" + configFile;
        String retstring = toolPath + "tools/mgodatagen" + " -a -f " + confDir
                + " -n " + workerNum + " --host=" + MongoDBHost + " --port="
                + MongoDBPort + " | grep -v 'MongoDB server version'";
        ssh.exec( retstring );
        System.out.println( "---exe: 数据已插入源端\n" );
    }

    // 迁移全量数据
    public static void genSequoiaShake( String confFile, Ssh ssh )
            throws Exception {
        String retstring = toolPath + "collector.linux_x86" + " -conf="
                + toolPath + confFile;
        ssh.exec( retstring );
        System.out.println( "---exe: 数据已迁移至目标端\n" );
    }

    // 迁移增量数据
    public static void genSequoiaShakeBackground( String confFile, Ssh ssh )
            throws Exception {
        boolean flag1 = true;
        String retstring = toolPath + "collector.linux_x86" + " -conf="
                + toolPath + confFile;
        CommLib.dropDatabaseDes( MongoDBHost, MongoDBPort, "sequoiashake" );
        ssh.execBackground( retstring );
        while ( flag1 ) {
            Thread.sleep( 1000 );
            ssh.exec( incrMonitor + " | grep -E 'logs_repl|logs_success' "
                    + "| awk -F ': ' '{print $2}' | awk -F ',' '{print $1}'" );
            String stdout1 = ssh.getStdout();
            for ( int i = 0; i < 3; i++ ) {
                Thread.sleep( 1000 );
                ssh.exec( incrMonitor + " | grep -E 'logs_repl|logs_success' "
                        + "| awk -F ': ' '{print $2}' | awk -F ',' '{print $1}'" );
                String stdout2 = ssh.getStdout();
                if ( stdout1.equals( stdout2 ) ) {
                    flag1 = false;
                } else {
                    flag1 = true;
                    break;
                }
            }
        }
        ssh.execBackground(
                "kill -9 `ps -ef | grep collector.linux_x86 | awk '{print $2}'`" );
        System.out.println( "---exe: 数据已迁移至目标端\n" );
    }

    public static void genSequoiaShakeBackground1( String confFile, Ssh ssh )
            throws Exception {
        boolean flag1 = true;
        String retstring = toolPath + "collector.linux_x86" + " -conf="
                + toolPath + confFile;
        CommLib.dropDatabaseDes( MongoDBHost, MongoDBPort, "sequoiashake" );
        ssh.execBackground( retstring );
        while ( flag1 ) {
            Thread.sleep( 1000 );
            ssh.exec( incrMonitor + " | grep -E 'logs_repl|logs_success' "
                    + "| awk -F ': ' '{print $2}' | awk -F ',' '{print $1}'" );
            String stdout1 = ssh.getStdout();
            for ( int i = 0; i < 3; i++ ) {
                Thread.sleep( 1000 );
                ssh.exec( incrMonitor + " | grep -E 'logs_repl|logs_success' "
                        + "| awk -F ': ' '{print $2}' | awk -F ',' '{print $1}'" );
                String stdout2 = ssh.getStdout();
                if ( stdout1.equals( stdout2 ) ) {
                    flag1 = false;
                } else {
                    flag1 = true;
                    break;
                }
            }
        }
        System.out.println( "---exe: 数据已迁移至目标端\n" );
    }

    // 判断增量是否完成同步
    public static void checkIncr( Ssh ssh ) throws Exception {
        boolean flag1 = true;
        while ( flag1 ) {
            Thread.sleep( 1000 );
            ssh.exec( incrMonitor + " | grep -E 'logs_repl|logs_success' "
                    + "| awk -F ': ' '{print $2}' | awk -F ',' '{print $1}'" );
            String stdout1 = ssh.getStdout();
            for ( int i = 0; i < 3; i++ ) {
                Thread.sleep( 1000 );
                ssh.exec( incrMonitor + " | grep -E 'logs_repl|logs_success' "
                        + "| awk -F ': ' '{print $2}' | awk -F ',' '{print $1}'" );
                String stdout2 = ssh.getStdout();
                if ( stdout1.equals( stdout2 ) ) {
                    flag1 = false;
                } else {
                    flag1 = true;
                    break;
                }
            }
        }
        System.out.println( "---exe: 数据已迁移至目标端\n" );
    }

    /**
     * #--excludeDbs 表示不对比的数据库，多个数据库用逗号分隔 #--excludeCollections
     * 表示不对比的集合，多个集合用逗号分隔 #--count 表示采样数据的数量 #--comparisonMode=no 表示使用统计信息对比记录数
     * #--comparisonMode=sample 表示只统计部分采样数据是否一致，采样数据由--count参数控制
     * #--comparisonMode=all 表示分批次对比所有文档是否一致(非常耗时)
     * <p>
     * 返回对比源端和目标端数据的执行命令
     */
    public static void checkDataResult( String comparisonMode,
            String excludeDbs, Ssh ssh ) throws Exception {
        String retstring = "python3 " + toolPath + "comparison.py" + " --src="
                + MongoDBHost + ":" + MongoDBPort + " --dest=" + SequoiaDBHost
                + ":" + SequoiaDBFapPort + " --excludeDbs=" + excludeDbs
                + " --comparisonMode=" + comparisonMode;
        ssh.exec( retstring );
        System.out.println( ssh.getStdout() );
    }

    // 从源端和目标端中删除数据库
    public static void dropDatabase( String dbName, MongoClient mongoClient,
            MongoClient sequoiaClient ) {
        mongoClient.getDatabase( dbName ).drop();
        sequoiaClient.getDatabase( dbName ).drop();
    }

    // 指定端口删除数据库
    public static void dropDatabaseDes( String host, int port, String dbName ) {
        String connection = "mongodb://admin:admin@" + host + ":" + port;
        MongoClient mongoClient = MongoClients.create( connection );
        mongoClient.getDatabase( dbName ).drop();
        mongoClient.close();
    }

    // 判断目标端不存在集合空间
    public static void checkCSNotExist( String dbName,
            MongoClient sequoiaClient ) {
        // 获取SequoiaDB中的所有数据库名称
        MongoIterable< String > dbNames = sequoiaClient.listDatabaseNames();
        // 判断是否存在数据库
        boolean dbExists = false;
        for ( String list : dbNames ) {
            if ( list.equals( dbName ) ) {
                dbExists = true;
                break;
            }
        }
        if ( dbExists ) {
            System.out
                    .println( "---exe: SequoiaDB contains the collectionSpace "
                            + dbName + "\n" );
            Assert.fail();
        } else {
            System.out.println(
                    "---exe: SequoiaDB does not contain the collectionSpace "
                            + dbName + "\n" );
        }
    }

    // 判断目标端不存在集合
    public static void checkCLNotExist( String dbName, String clName,
            MongoClient sequoiaClient ) {
        // 获取SequoiaDB中的所有数据库名称
        MongoIterable< String > clNames = sequoiaClient.getDatabase( dbName )
                .listCollectionNames();
        // 判断是否存在数据库
        boolean clExists = false;
        for ( String list : clNames ) {
            if ( list.equals( clName ) ) {
                clExists = true;
                break;
            }
        }
        if ( clExists ) {
            System.out.println( "---exe: SequoiaDB contains the collection "
                    + dbName + "." + clName + "\n" );
            Assert.fail();
        } else {
            System.out.println(
                    "---exe: SequoiaDB does not contain the collection "
                            + dbName + "." + clName + "\n" );
        }
    }

    // 对比源端和目标端指定库下所有集合的索引
    public static void checkIndex( String dbName, MongoClient mongoClient,
            MongoClient sequoiaClient ) {
        MongoDatabase mgoCSName = mongoClient.getDatabase( dbName );
        MongoDatabase seqCSName = sequoiaClient.getDatabase( dbName );
        MongoIterable< String > mgoClNames = mgoCSName.listCollectionNames();
        MongoIterable< String > seqClNames = seqCSName.listCollectionNames();
        for ( String mgoClName : mgoClNames ) {
            MongoCollection< Document > mgoCollection = mgoCSName
                    .getCollection( mgoClName );
            MongoCursor< Document > mgoIndexCursor = mgoCollection.listIndexes()
                    .iterator();
            while ( mgoIndexCursor.hasNext() ) {
                Document document1 = mgoIndexCursor.next();
                for ( String seqClName : seqClNames ) {
                    if ( mgoClName.equals( seqClName ) ) {
                        MongoCollection< Document > seqCollection = seqCSName
                                .getCollection( seqClName );
                        MongoCursor< Document > seqIndexCursor = seqCollection
                                .listIndexes().iterator();
                        while ( seqIndexCursor.hasNext() ) {
                            Document document2 = seqIndexCursor.next();
                            if ( document1.getString( "name" )
                                    .equals( document2.getString( "name" ) ) ) {
                                if ( !document1.get( "key" )
                                        .equals( document2.get( "key" ) ) ) {
                                    Assert.fail(
                                            "---exe: The indexes on a collectionSpace are not the same: [ src: "
                                                    + document1.get( "key" )
                                                    + " ] <=> [ dest: "
                                                    + document2.get( "key" )
                                                    + " ]" );
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
        System.out.println(
                "---exe: All indexes on a collectionSpace are the same\n" );
    }

    // 判断目标端索引都为普通升序索引
    public static void checkIndexNotExist( String dbName, String clName,
            MongoClient sequoiaClient ) {
        MongoCursor< Document > seqIndexCursor = sequoiaClient
                .getDatabase( dbName ).getCollection( clName ).listIndexes()
                .iterator();
        while ( seqIndexCursor.hasNext() ) {
            String key = String.valueOf( seqIndexCursor.next().get( "key" ) );
            if ( !key.contains( "1" ) ) {
                Assert.fail(
                        "---exe: The indexes on a collection are not normal index\n" );
            }
        }
        System.out.println(
                "---exe: All indexes on a collection are normal index\n" );
    }

    // 判断数据是否符合预期
    public static void checkRecordEqual( String dbName, String clName,
            MongoClient mongoClient, MongoClient sequoiaClient ) {
        long srcCount = mongoClient.getDatabase( dbName )
                .getCollection( clName ).countDocuments();
        long destCount = sequoiaClient.getDatabase( dbName )
                .getCollection( clName ).countDocuments();
        if ( srcCount != destCount ) {
            Assert.fail( "---exe: The src count is " + srcCount
                    + " The dest count is " + destCount + " not equal\n" );
        } else {
            System.out.println( "---exe: The src count is " + srcCount
                    + " The dest count is " + destCount + " equal\n" );
        }
    }

    public static void checkRecordNotEqual( String dbName, String clName,
            MongoClient mongoClient, MongoClient sequoiaClient ) {
        long srcCount = mongoClient.getDatabase( dbName )
                .getCollection( clName ).countDocuments();
        long destCount = sequoiaClient.getDatabase( dbName )
                .getCollection( clName ).countDocuments();
        if ( srcCount == destCount ) {
            Assert.fail( "---exe: The src count is " + srcCount
                    + " The dest count is " + destCount + " equal\n" );
        } else {
            System.out.println( "---exe: The src count is " + srcCount
                    + " The dest count is " + destCount + " not equal\n" );
        }
    }

    // 修改conf配置文件的checkpoint.start_position参数
    public static void updateCheckpoint( String confFile, Ssh ssh )
            throws Exception {
        ssh.exec( "date -u +\"%Y-%m-%dT%H:%M:%SZ\"" );
        String nowDate = ssh.getStdout().trim();
        ssh.exec(
                "sed -i 's/checkpoint.start_position =.*/checkpoint.start_position = "
                        + nowDate + "/g' " + toolPath + confFile );
    }

    // 判断源端mongodb部署模式并且修改对应配置信息
    public static boolean checkMongoMode( String confFile, Ssh ssh,
            MongoClient mongoClient ) throws Exception {
        boolean flag = false;
        long count = mongoClient.getDatabase( "config" )
                .getCollection( "shards" ).countDocuments();
        if ( count == 0 ) {
            ssh.exec(
                    "sed -i 's/filter.ddl_enable =.*/filter.ddl_enable = true/g' "
                            + toolPath + confFile );
            flag = true;
            System.out.println(
                    "---exe: The source database is in standalone or replica set\n" );
        } else {
            ssh.exec(
                    "sed -i 's/filter.ddl_enable =.*/filter.ddl_enable = false/g' "
                            + toolPath + confFile );
            MongoIterable< String > databaseNames = mongoClient
                    .listDatabaseNames();
            for ( String dbName : databaseNames ) {
                if ( !dbName.equals( "admin" ) && !dbName.equals( "config" )
                        && !dbName.equals( "local" ) ) {
                    MongoDatabase database = mongoClient.getDatabase( dbName );
                    MongoIterable< String > collectionNames = database
                            .listCollectionNames();
                    for ( String collectionName : collectionNames ) {
                        MongoCollection< Document > collection = database
                                .getCollection( collectionName );
                        collection.dropIndexes();
                    }
                }
            }
            System.out.println(
                    "---exe: The source database is in shard cluster\n" );
        }
        return flag;
    }

    // mongodb创建用户，指定用户名，密码，权限
    public static void createUser( MongoClient mongoClient, String username,
            String password, String roles ) {
        MongoDatabase adminDb = mongoClient.getDatabase( "admin" );
        adminDb.runCommand( new Document( "createUser", username )
                .append( "pwd", password ).append( "roles",
                        Arrays.asList(
                                new Document( "role", roles ).append( "db",
                                        "admin" ),
                                new Document( "role", "clusterAdmin" )
                                        .append( "db", "admin" ) ) ) );
        System.out.println(
                "---exe: User created successfully: " + username + "\n" );
    }

    // 自定义roles
    public static void createRole( MongoClient mongoClient, String rolename,
            List< String > actions ) {
        MongoDatabase adminDb = mongoClient.getDatabase( "admin" );
        adminDb.runCommand( new Document( "createRole", rolename ).append(
                "privileges",
                Arrays.asList( new Document( "actions", actions ).append(
                        "resource", new Document( "anyResource", true ) ) ) )
                .append( "roles", Arrays.asList() ) );
        System.out.println(
                "---exe: Role created successfully: " + rolename + "\n" );
    }

    // mongodb创建删除用户
    public static void dropUser( MongoClient mongoClient, String username ) {
        MongoDatabase adminDb = mongoClient.getDatabase( "admin" );
        adminDb.runCommand( new Document( "dropUser", username ) );
        System.out.println(
                "---exe: User deleted successfully: " + username + "\n" );
    }

    // mongodb创建删除角色
    public static void dropRole( MongoClient mongoClient, String rolename ) {
        MongoDatabase adminDb = mongoClient.getDatabase( "admin" );
        adminDb.runCommand( new Document( "dropRole", rolename ) );
        System.out.println(
                "---exe: Role deleted successfully: " + rolename + "\n" );
    }
}
