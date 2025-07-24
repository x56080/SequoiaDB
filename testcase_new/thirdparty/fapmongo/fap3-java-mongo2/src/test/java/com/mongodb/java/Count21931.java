package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.MongoCommandException;
import com.mongodb.QueryBuilder;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21931:count操作
 * @author fanyu
 * @Date 2020/3/17
 * @version 1.00
 */
public class Count21931 extends MongodbTestBase {
    private DB db;
    private String clName;
    private DBCollection cl;
    // 不能小于10
    private int num = 10;
    private List< DBObject > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        clName = javaDBNameWithVersion + "_cl21931";
        list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", "" + i )
                    .append( "c", new int[] { i + 1, i + 2, i + 3 } )
                    .append( "d", "test-" + i + "-" + i % 3 ) );
        }
        cl = db.getCollection( clName );
        cl.insert( list );
        cl.createIndex( new BasicDBObject( "a", 1 ), "a" );
        cl.createIndex( new BasicDBObject( "b", 1 ).append( "f", 1 ), "bf" );
    }

    @Test
    public void test1() {
        long actCount;
        // 不带条件count
        actCount = cl.getCount();
        Assert.assertEquals( actCount, num );

        actCount = cl.count();
        Assert.assertEquals( actCount, num );

        // 带空条件count
        actCount = cl.count( new BasicDBObject() );
        Assert.assertEquals( actCount, num );

        actCount = cl.getCount( new BasicDBObject() );
        Assert.assertEquals( actCount, num );

        // 带条件is + lt + gte
        DBObject query5 = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThanEquals( num / 2 ).get();
        actCount = cl.count( query5 );
        Assert.assertEquals( actCount, num / 2 + 1 );

        // 带条件in + hint 没有找到对应的接口
        DBObject query6 = QueryBuilder.start( "a" ).in( new int[] { 1, 2, 3 } )
                .get();
        actCount = cl.getCount( query6, new BasicDBObject( "hint", "a" ) );
        Assert.assertEquals( actCount, 3 );

        actCount = cl.getCount( query6 );
        Assert.assertEquals( actCount, 3 );

        // sequoiadb对filed、skip limit不生效
        actCount = cl.getCount( query6, new BasicDBObject( "b", 0 ) );
        Assert.assertEquals( actCount, 3 );

        // 集合不存在数据，进行count
        cl.remove( new BasicDBObject() );
        actCount = cl.count( new BasicDBObject() );
        Assert.assertEquals( actCount, 0 );

        // 参数校验
        try {
            cl.count( new BasicDBObject( "$gt", 1 ) );
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
