package com.mongodb.utils;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;
import org.springframework.data.mongodb.MongoDbFactory;
import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.data.mongodb.core.convert.MongoConverter;
import org.springframework.data.mongodb.gridfs.GridFsTemplate;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.testng.AbstractTestNGSpringContextTests;
import org.testng.ITestContext;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.BeforeSuite;

import com.mongodb.DB;
import com.mongodb.Mongo;
import com.mongodb.MongoClient;
import com.mongodb.MongoClientOptions;
import com.mongodb.MongoOptions;
import com.mongodb.ServerAddress;

@SuppressWarnings("deprecation")
@ContextConfiguration(locations = { "classpath*:spring-mongodb-2.xml" })
public class MongodbTestBase extends AbstractTestNGSpringContextTests {
    public static String javaMongoVersion;
    public static String springMongoVersion;
    private static String javaDBName;
    private static String springDBName;
    public static String javaDBNameWithVersion;
    public static String springDBNameWithVersion;

    public static MongoClient client;
    public static Mongo springMongoClient;
    @Autowired
    public MongoDbFactory mongoDbFactory;
    @Autowired
    public MongoConverter converter;
    @Autowired
    public MongoTemplate mongoTemplate;

    @Autowired
    // info1: 用例里面用的默认桶是mongodb自带的默认桶，非xml定义的桶
    // info2: 此处为spring-mongodb-2.xml注入，bucket在xml文件定义，目前暂时没有用例用到，先保留后面看情况再做优化
    public GridFsTemplate gridFsTemplate;

    @Autowired
    public Config config;
    private Config springConfig;

    /**
     * @Description: 初始化测试套
     * @throws UnknownHostException
     */
    @BeforeSuite(alwaysRun = true)
    public void initSuite() throws UnknownHostException {
        @SuppressWarnings("resource")
        ApplicationContext ctx = new ClassPathXmlApplicationContext(
                "spring-mongodb-2.xml" );
        springConfig = ( Config ) ctx.getBean( "config" );
        springMongoClient = ( Mongo ) ctx.getBean( "mongo" );

        javaMongoVersion = springConfig.getJavaMongoVersion();
        springMongoVersion = springConfig.getSpringMongoVersion();

        javaDBName = springConfig.getJavaDBName();
        springDBName = springConfig.getSpringDBName();

        javaDBNameWithVersion = javaDBName + "_" + javaMongoVersion;
        springDBNameWithVersion = springDBName + "_" + springMongoVersion;

        client = getClient( springConfig.getUrls(),
                springMongoClient.getMongoOptions() );

        cleanEnv();
    }

    /**
     * @Description 结束测试套，如果有用例失败，不会清理环境； 如果用例全部执行成功，会清理环境
     * @param context
     *                    testng上下文
     * @throws Exception
     */
    @AfterSuite(alwaysRun = true)
    public void finiSuite( ITestContext context ) throws Exception {
        try {
            if ( context.getFailedTests().size() == 0
                    && context.getSkippedTests().size() == 0 ) {
                cleanEnv();
            }
        } finally {
            client.close();
            springMongoClient.close();
        }
    }

    /**
     * @Description: 获取mongodb客户端
     * @param hostname
     *                     主机名
     * @param port
     *                     端口号
     * @return
     * @throws UnknownHostException
     */
    public static MongoClient getClient( String hostname, int port )
            throws UnknownHostException {
        return getClient( new String[] { hostname + ":" + port },
                springMongoClient.getMongoOptions() );
    }

    /**
     * @Description: 获取mongodb客户端
     * @return
     * @throws UnknownHostException
     */
    public static MongoClient getClient( String[] urls, MongoOptions options )
            throws UnknownHostException {
        List< ServerAddress > serverAddressList = new ArrayList<>();
        for ( String url : urls ) {
            serverAddressList.add( new ServerAddress( url.split( ":" )[ 0 ],
                    Integer.parseInt( url.split( ":" )[ 1 ] ) ) );
        }

        MongoClientOptions clientOptions = MongoClientOptions.builder()
                .connectionsPerHost( options.getConnectionsPerHost() )
                .connectTimeout( options.getConnectTimeout() )
                .threadsAllowedToBlockForConnectionMultiplier( options
                        .getThreadsAllowedToBlockForConnectionMultiplier() )
                .maxWaitTime( options.getMaxWaitTime() ).build();
        return new MongoClient( serverAddressList, clientOptions );
    }

    /**
     * @Description: 获取默认的mongodb数据库
     * @return
     */
    public static DB getDB( MongoClient client ) {
        return client.getDB( javaDBNameWithVersion );
    }

    /**
     * @Description: 删除集合
     * @param db
     * @param clNames
     */
    public static void dropCL( DB db, String... clNames ) {
        for ( String clName : clNames ) {
            if ( db.collectionExists( clName ) ) {
                db.getCollection( clName ).drop();
            }
        }
    }

    /**
     * @Description 使用mongoTemplate删除集合
     * @param clNames
     */
    public static void dropCL( MongoTemplate mongoTemplate,
            String... clNames ) {
        for ( String clName : clNames ) {
            if ( mongoTemplate.collectionExists( clName ) ) {
                mongoTemplate.dropCollection( clName );
            }
        }
    }

    /**
     * @Description 根据每个用例测试结果决定是否删除cl。用例失败则不删除；用例成功则删除
     * @param context
     *                        testng上下文
     * @param classString
     *                        类的toString，如：this.toString()
     * @param db
     *                        数据库实例
     * @param clNames
     *                        集合
     */
    public static void dropCLByTestResult( ITestContext context,
            String classString, DB db, String... clNames ) {
        if ( !context.getFailedTests().getAllResults().toString()
                .contains( classString )
                && !context.getSkippedTests().toString()
                        .contains( classString ) ) {
            dropCL( db, clNames );
        }
    }

    /**
     * @Description 根据每个用例测试结果删除cl。用例失败则不删除；用例成功则删除
     * @param context
     *                        testng上下文
     * @param classString
     *                        类的toString，如：this.toString()
     * @param clNames
     *                        集合名
     */
    public static void dropCLByTestResult( ITestContext context,
            String classString, MongoTemplate mongoTemplate,
            String... clNames ) {

        if ( !context.getFailedTests().getAllResults().toString()
                .contains( classString )
                && !context.getSkippedTests().toString()
                        .contains( classString ) ) {
            dropCL( mongoTemplate, clNames );
        }
    }

    /**
     * @Description: 清理环境
     * @throws UnknownHostException
     */
    public void cleanEnv() throws UnknownHostException {
        try {
            DB db = client.getDB( javaDBNameWithVersion );
            db.dropDatabase();
        } catch ( Exception e ) {
            e.printStackTrace();
            logger.error( e.getMessage() );
        }
    }
}
