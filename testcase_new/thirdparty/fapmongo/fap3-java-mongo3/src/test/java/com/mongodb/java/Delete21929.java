package com.mongodb.java;

import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;

import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.regex.Pattern;

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
import com.mongodb.client.result.DeleteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21929:delete操作
 * @author fanyu
 * @Date 2020/3/23
 * @version 1.00
 */
public class Delete21929 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    // 不能小于10
    private int num = 10;
    private List< Document > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl21929";
        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i ).append( "b", "" + i )
                    .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
                    .append( "d", "test-" + i + "-" + i % 3 ) );
        }
        cl = db.getCollection( clName );
        cl.insertMany( list );
    }

    @Test
    public void test1() {
        Bson query;
        DeleteResult result;
        // 删除单条记录
        query = Filters.eq( "a", 0 );
        Assert.assertEquals( cl.count( query ), 1 );
        result = cl.deleteOne( query );
        Assert.assertEquals( result.getDeletedCount(), 1 );
        Assert.assertEquals( cl.count( query ), 0 );

        // 删除多条记录
        // lt gte
        query = Filters.and( lt( "a", 2 * num / 3 ), gte( "a", num / 3 ) );
        Assert.assertEquals( cl.count( query ), 2 * num / 3 - num / 3 );
        result = cl.deleteMany( query );
        Assert.assertEquals( result.getDeletedCount(), 2 * num / 3 - num / 3 );
        Assert.assertEquals( cl.count( query ), 0 );

        // $regex
        Pattern pattern = Pattern.compile( "test-" + 2 * num / 3 + "-.*" );
        query = Filters.regex( "d", pattern );
        Assert.assertEquals( cl.count( query ), 1 );
        result = cl.deleteMany( query );
        Assert.assertEquals( result.getDeletedCount(), 1 );
        Assert.assertEquals( cl.count( query ), 0 );

        // 匹配不到记录，删除记录
        query = Filters.gt( "a", num );
        Assert.assertEquals( cl.count( query ), 0 );
        result = cl.deleteMany( query );
        Assert.assertEquals( result.getDeletedCount(), 0 );
        Assert.assertEquals( cl.count( query ), 0 );

        // 字段不存在，删除记录
        query = Filters.gt( "k", num );
        Assert.assertEquals( cl.count( query ), 0 );
        result = cl.deleteMany( query );
        Assert.assertEquals( result.getDeletedCount(), 0 );
        Assert.assertEquals( cl.count( query ), 0 );

        // 带空bson，删除所有记录
        long count = cl.count();
        result = cl.deleteMany( new Document() );
        Assert.assertEquals( result.getDeletedCount(), count );
        Assert.assertEquals( cl.count(), 0 );

        // 集合不存在记录，删除记录
        result = cl.deleteMany( lt( "a", num ) );
        Assert.assertEquals( result.getDeletedCount(), 0 );

        // 参数校验
        try {
            cl.deleteMany( lt( "$", null ) );
            Assert.fail( "exp fail but act success!!!" );
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
