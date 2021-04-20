package com.mongodb.utils;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.apache.log4j.Logger;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;
import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.testng.AbstractTestNGSpringContextTests;
import org.testng.ITestContext;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.BeforeSuite;

import com.mongodb.DB;
import com.mongodb.MongoClient;
import com.mongodb.MongoClientOptions;
import com.mongodb.ServerAddress;
import com.mongodb.client.MongoDatabase;

@ContextConfiguration(locations = { "classpath*:spring-mongodb-3x1.xml" })
public class MongodbTestBase extends AbstractTestNGSpringContextTests {
    private static final Logger logger = Logger
            .getLogger( MongodbTestBase.class );

    public static String javaMongoVersion;
    public static String springMongoVersion;
    private static String javaDBName;
    private static String springDBName;
    public static String javaDBNameWithVersion;
    public static String springDBNameWithVersion;

    public static MongoClient client;
    public static MongoClient springMongoClient;
    private Config springConfig;
    @Autowired
    public MongoTemplate mongoTemplate;
    @Autowired
    public Config config;

    /**
     * @Description: 初始化测试套
     * @throws UnknownHostException
     */
    @BeforeSuite(alwaysRun = true)
    public void initSuite() throws UnknownHostException {
        @SuppressWarnings("resource")
        ApplicationContext ctx = new ClassPathXmlApplicationContext(
                "spring-mongodb-3x1.xml" );
        springConfig = ( Config ) ctx.getBean( "config" );
        springMongoClient = ( MongoClient ) ctx.getBean( "mongo" );

        javaMongoVersion = springConfig.getJavaMongoVersion();
        springMongoVersion = springConfig.getSpringMongoVersion();

        javaDBName = springConfig.getJavaDBName();
        springDBName = springConfig.getSpringDBName();

        javaDBNameWithVersion = javaDBName + "_" + javaMongoVersion;
        springDBNameWithVersion = springDBName + "_" + springMongoVersion;

        client = getClient( springConfig.getUrls(),
                springMongoClient.getMongoClientOptions() );
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
        return new MongoClient( new ServerAddress( hostname, port ),
                springMongoClient.getMongoClientOptions() );
    }

    /**
     * @Description: 获取mongodb客户端
     * @return
     * @throws UnknownHostException
     */
    public static MongoClient getClient( String[] urls,
            MongoClientOptions options ) throws UnknownHostException {
        List< ServerAddress > serverAddressList = new ArrayList<>();
        for ( String url : urls ) {
            serverAddressList.add( new ServerAddress( url.split( ":" )[ 0 ],
                    Integer.parseInt( url.split( ":" )[ 1 ] ) ) );
        }
        return new MongoClient( serverAddressList, options );
    }

    /**
     * @Description: 获取默认mongodb数据库
     * @return
     * @throws UnknownHostException
     */
    public static MongoDatabase getDataBase( MongoClient client ) {
        return client.getDatabase( javaDBNameWithVersion );
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
     * @Description: 使用db删除集合
     * @param db
     * @param clNames
     */
    public static void dropCL( MongoDatabase db, String... clNames ) {
        List< String > actClNames = db.listCollectionNames()
                .into( new ArrayList< String >() );
        for ( String clName : clNames ) {
            if ( actClNames.contains( clName ) ) {
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
            String classString, MongoDatabase db, String... clNames ) {
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
            MongoDatabase db = springMongoClient
                    .getDatabase( springDBNameWithVersion );
            db.drop();
        } catch ( Exception e ) {
            e.printStackTrace();
            logger.error( e.getMessage() );
        }
    }
}
