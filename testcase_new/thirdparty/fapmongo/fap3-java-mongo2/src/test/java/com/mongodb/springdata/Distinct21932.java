package com.mongodb.springdata;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.bson.BasicBSONObject;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.MongoCommandException;
import com.mongodb.ReadPreference;
import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21932:distinct操作
 * @author fanyu
 * @Date 2020/3/19
 * @version 1.00
 */
public class Distinct21932 extends MongodbTestBase {
    private int num = 10;
    private List< Entity > list;
    private String clName;

    @BeforeClass
    public void setUp() {
        clName = springDBNameWithVersion + "_cl21932";
        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            for ( int j = 0; j < 2; j++ ) {
                list.add( new Entity( "a" + i,
                        Entity.SEXS[ i % Entity.SEXS.length ], i, i,
                        Entity.COURSES ) );
            }
        }
        mongoTemplate.insert( list, clName );
    }

    @Test
    @SuppressWarnings("rawtypes")
    public void test1() {
        List distinctResult;
        // 不带条件去重
        DBCollection cl = mongoTemplate.getCollection( clName );
        // 无重复值
        distinctResult = cl.distinct( "age" );
        Assert.assertEquals( distinctResult.size(), num );
        for ( int i = 0; i < distinctResult.size(); i++ ) {
            Assert.assertEquals( distinctResult.get( i ), i );
        }
        // 有重复值
        distinctResult = cl.distinct( "sex" );
        Assert.assertEquals( distinctResult, Arrays.asList( Entity.SEXS ) );

        // 带条件去重
        // 无重复值
        DBObject options = new BasicDBObject( "age",
                new BasicBSONObject( "$gte", num / 2 ) );
        distinctResult = cl.distinct( "age", options );
        for ( int i = 0; i < ( num - num / 2 ); i++ ) {
            Assert.assertEquals( distinctResult.get( i ), num / 2 + i );
        }

        // 有重复值
        BasicDBObject query = new BasicDBObject( "age",
                new BasicBSONObject( "$lte", num / 3 ) );
        distinctResult = cl.distinct( "sex", query );
        Assert.assertEquals( distinctResult, Arrays.asList( Entity.SEXS ) );

        // 匹配不到记录
        BasicDBObject query2 = new BasicDBObject( "age",
                new BasicBSONObject( "$gt", num + 10 ) );
        distinctResult = cl.distinct( "age", query2 );
        Assert.assertEquals( distinctResult.size(), 0 );

        // 对不存在的字段进行去重
        distinctResult = cl.distinct( "age1" );
        Assert.assertEquals( distinctResult.size(), 0 );

        // 带有ReadConcern去重
        distinctResult = cl.distinct( "age", query, ReadPreference.primary() );
        for ( int i = 0; i < distinctResult.size(); i++ ) {
            Assert.assertEquals( distinctResult.get( i ), i );
        }

        // 空集合进行去重
        Query query1 = new Query();
        mongoTemplate.remove( query1, Entity.class, clName );
        distinctResult = cl.distinct( "age" );
        Assert.assertEquals( distinctResult.size(), 0 );

        // 参数校验
        try {
            cl.distinct( "sex", new BasicDBObject( "$gt", 1 ) );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }
}
