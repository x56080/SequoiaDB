package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.ReadPreference;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description sseqDB-21932:distinct操作
 * @author fanyu
 * @Date 2020/3/12
 * @version 1.00
 */

public class Distinct21932 extends MongodbTestBase {
    private DB db;
    private String clName;
    private DBCollection cl;
    // 不能小于10
    private int num = 10;
    private List< DBObject > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        clName = javaDBNameWithVersion + "_cl21932";

        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i % 3 ).append( "b", "" + i % 3 )
                    .append( "c", new int[] { i % 3 } ).append( "d", i ) );
        }
        cl = db.getCollection( clName );
        cl.insert( list );
        cl.createIndex( new BasicDBObject( "a", 1 ), "a" );
        cl.createIndex( new BasicDBObject( "b", 1 ).append( "f", 1 ), "bf" );
    }

    @SuppressWarnings("rawtypes")
    @Test
    public void test1() {
        BasicDBObject options;
        List actResult;
        List< Integer > expList = Arrays.asList( 0, 1, 2 );
        // 不带条件去重
        actResult = cl.distinct( "a" );
        Assert.assertEquals( actResult, expList );

        // 带条件去重
        // 匹配到记录
        options = new BasicDBObject( "d",
                new BasicBSONObject( "$gte", 0 ).append( "$lt", 2 ) );
        actResult = cl.distinct( "a", options );
        List< Integer > expList2 = Arrays.asList( 0, 1 );
        Assert.assertEquals( actResult, expList2 );

        options = new BasicDBObject( "a",
                new BasicBSONObject( "$lte", num ).append( "$gte", 0 ) );
        actResult = cl.distinct( "a", options );
        List< Integer > expList3 = Arrays.asList( 0, 1, 2 );
        Assert.assertEquals( actResult, expList3 );

        // 匹配不到记录
        options = new BasicDBObject( "a", new BasicBSONObject( "$lte", -1 ) );
        actResult = cl.distinct( "a", options );
        Assert.assertEquals( actResult.size(), 0 );

        // 带有ReadConcern,sequoiadb目前不支持ReadConcern
        actResult = cl.distinct( "a", options, ReadPreference.primary() );
        Assert.assertEquals( actResult.size(), 0 );

        // 对不存在的字段去重
        actResult = cl.distinct( "a1" );
        Assert.assertEquals( actResult.size(), 0 );

        // 集合不存在记录
        cl.remove( new BasicDBObject() );
        actResult = cl.distinct( "a" );
        Assert.assertEquals( actResult.size(), 0 );

        // 参数校验
        try {
            cl.distinct( "a", new BasicDBObject( "$gt", 0 ) );
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
