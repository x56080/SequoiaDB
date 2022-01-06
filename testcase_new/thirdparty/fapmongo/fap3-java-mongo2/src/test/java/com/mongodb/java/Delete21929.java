package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.regex.Pattern;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.QueryBuilder;
import com.mongodb.WriteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21929:delete操作
 * @author fanyu
 * @Date 2020/3/12
 * @version 1.00
 */
public class Delete21929 extends MongodbTestBase {
    private DB db;
    private String clName;
    private DBCollection cl;
    // 不能小于10
    private int num = 10;
    private List< DBObject > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        clName = javaDBNameWithVersion + "_cl21929";

        list = new CopyOnWriteArrayList<>();
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
        // 删除单条记录
        DBObject query1 = QueryBuilder.start( "a" ).is( 0 ).get();
        WriteResult result1 = cl.remove( query1 );
        Assert.assertEquals( result1.getN(), 1 );
        Assert.assertEquals( cl.count( query1 ), 0 );

        // 删除多条记录
        // lt gte
        DBObject query2 = QueryBuilder.start( "a" ).greaterThanEquals( 1 )
                .lessThan( num / 3 ).get();
        WriteResult result2 = cl.remove( query2 );
        Assert.assertEquals( result2.getN(), num / 3 - 1 );
        Assert.assertEquals( cl.count( query2 ), 0 );

        // $regex
        Pattern pattern = Pattern.compile( "test-" + 2 * num / 3 + "-.*" );
        DBObject query3 = QueryBuilder.start( "d" ).regex( pattern ).get();
        WriteResult result3 = cl.remove( query3 );
        Assert.assertEquals( result3.getN(), 1 );
        Assert.assertEquals( cl.count( query3 ), 0 );

        // 匹配不到记录，删除记录
        DBObject query4 = QueryBuilder.start( "a" ).greaterThan( num ).get();
        WriteResult result4 = cl.remove( query4 );
        Assert.assertEquals( result4.getN(), 0 );
        Assert.assertEquals( cl.count( query4 ), 0 );

        // 字段不存在，删除记录
        DBObject query5 = QueryBuilder.start( "k" ).greaterThan( num ).get();
        WriteResult result5 = cl.remove( query5 );
        Assert.assertEquals( result5.getN(), 0 );
        Assert.assertEquals( cl.count( query5 ), 0 );

        // 带空bson，删除所有记录
        long count = cl.count();
        WriteResult result6 = cl.remove( new BasicDBObject() );
        Assert.assertEquals( result6.getN(), count );
        Assert.assertEquals( cl.count( new BasicDBObject() ), 0 );

        // 集合不存在记录，删除记录
        WriteResult result7 = cl.remove( new BasicDBObject( "a", 1 ) );
        Assert.assertEquals( result7.getN(), 0 );

        try {
            cl.remove( new BasicDBObject( "$", 1 ) );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-6" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
