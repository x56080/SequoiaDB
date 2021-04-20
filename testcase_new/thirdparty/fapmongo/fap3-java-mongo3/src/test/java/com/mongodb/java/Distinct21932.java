package com.mongodb.java;

import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

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
import com.mongodb.client.model.Filters;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21932:distinct操作
 * @author fanyu
 * @Date 2020/3/23
 * @version 1.00
 */
public class Distinct21932 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    // 不能小于10
    private int num = 10;
    private List< Document > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl21932";
        list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i % 3 ).append( "b", "" + i % 3 )
                    .append( "c", Collections.singletonList( i % 3 ) )
                    .append( "d", i ) );
        }
        cl = db.getCollection( clName );
        cl.insertMany( list );
    }

    @Test
    public void test1() {
        Bson query;
        Collection< Integer > actResult;
        List< Integer > expList = Arrays.asList( 0, 1, 2 );
        // 不带条件去重
        actResult = cl.distinct( "a", Integer.class )
                .into( new ArrayList< Integer >() );
        Assert.assertEquals( actResult, expList );

        // 带条件去重
        // 匹配到记录
        query = Filters.and( gte( "d", 0 ), lt( "d", 2 ) );
        actResult = cl.distinct( "a", query, Integer.class )
                .into( new ArrayList< Integer >() );
        List< Integer > expList2 = Arrays.asList( 0, 1 );
        Assert.assertEquals( actResult, expList2 );

        query = Filters.and( gte( "a", 0 ), lt( "a", num ) );
        actResult = cl.distinct( "a", query, Integer.class )
                .into( new ArrayList< Integer >() );
        List< Integer > expList3 = Arrays.asList( 0, 1, 2 );
        Assert.assertEquals( actResult, expList3 );

        // 匹配不到记录
        query = Filters.and( lt( "a", 0 ) );
        actResult = cl.distinct( "a", query, Integer.class )
                .into( new ArrayList< Integer >() );
        Assert.assertEquals( actResult.size(), 0, actResult.toString() );

        // 对不存在的字段去重
        actResult = cl.distinct( "a1", Integer.class )
                .into( new ArrayList< Integer >() );
        Assert.assertEquals( actResult.size(), 0, actResult.toString() );

        // 集合不存在记录
        cl.deleteMany( new Document() );
        actResult = cl.distinct( "a", Integer.class )
                .into( new ArrayList< Integer >() );
        Assert.assertEquals( actResult.size(), 0, actResult.toString() );

        // 参数校验
        try {
            cl.distinct( "$", new Document( "$eq", 1 ), Integer.class )
                    .into( new ArrayList< Integer >() );
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
