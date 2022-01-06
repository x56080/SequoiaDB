package com.mongodb.java;

import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;

import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.MongoCommandException;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.CountOptions;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21931:count操作
 * @author fanyu
 * @Date 2020/3/23
 * @version 1.00
 */
public class Count21931 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    // 不能小于10
    private int num = 10;
    private List< Document > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl21931";
        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i ).append( "b", "" + i )
                    .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
                    .append( "d", "test-" + i + "-" + i % 3 ) );
        }
        cl = db.getCollection( clName );
        cl.insertMany( list );
        cl.createIndex( Indexes.ascending( "a" ),
                new IndexOptions().unique( false ).name( "a" ) );
        cl.createIndex( Indexes.ascending( "b" ),
                new IndexOptions().unique( false ).name( "b" ) );
    }

    @Test
    public void test1() {
        long actCount;
        // 不带条件count
        actCount = cl.count();
        Assert.assertEquals( actCount, num );

        // 带空条件count
        actCount = cl.count( new Document() );
        Assert.assertEquals( actCount, num );

        // 带条件is + lt + gte
        Bson query4 = Filters.and( gte( "a", 0 ), lt( "a", num / 2 ) );
        actCount = cl.count( query4 );
        Assert.assertEquals( actCount, num / 2 );

        // 带条件in + hint sequoiadb对filed、skip limit不生效
        // 索引存在，hint的参数值为索引名
        Bson query5 = Filters.in( "b", "1", "2", "3" );
        actCount = cl.count( query5, new CountOptions().hintString( "a" ) );
        Assert.assertEquals( actCount, 3 );

        // 索引存在，hint的参数值为键值对
        Bson query6 = Filters.in( "b", "1", "2", "3" );
        actCount = cl.count( query6,
                new CountOptions().hint( new Document( "a", 1 ) ) );
        Assert.assertEquals( actCount, 3 );

        // 索引不存在，hint的参数值为索引名
        Bson query7 = Filters.in( "b", "1", "2", "3" );
        actCount = cl.count( query7,
                new CountOptions().hintString( "a-inexistences" ) );
        Assert.assertEquals( actCount, 3 );

        // 索引不存在，hint的参数值为键值对方式
        Bson query8 = Filters.in( "b", "1", "2", "3" );
        actCount = cl.count( query8, new CountOptions()
                .hint( new Document( "a-inexistences", 1 ) ) );
        Assert.assertEquals( actCount, 3 );

        // 集合不存在数据，进行count
        cl.deleteMany( new Document() );
        actCount = cl.count( new Document() );
        Assert.assertEquals( actCount, 0 );

        // 参数校验
        try {
            cl.count( new Document( "$gt", 1 ) );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
