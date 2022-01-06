package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.MongoCommandException;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.client.model.Sorts;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21926:insert操作
 * @author fanyu
 * @Date 2020/3/23
 * @version 1.00
 */
public class Insert21926 extends MongodbTestBase {
    private MongoDatabase db;
    private String[] clNames = new String[ 2 ];

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clNames[ 0 ] = javaDBNameWithVersion + "_cl21926A";
        clNames[ 1 ] = javaDBNameWithVersion + "_cl21926B";
    }

    @Test
    public void test1() {
        MongoCollection< Document > cl = db.getCollection( clNames[ 0 ] );
        // 创建普通索引
        cl.createIndex( Indexes.ascending( "a" ),
                new IndexOptions().unique( false ).name( "a" ) );
        // 准备记录
        int num = 10;
        List< Document > list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i ).append( "b", i - 1 ) );
        }
        // 插入单条数据
        cl.insertOne( list.get( 0 ) );
        // 去除"_id"字段，因为_id是唯一索引
        list.get( 0 ).remove( "_id" );
        // 重复插入单条数据
        cl.insertOne( list.get( 0 ) );
        // 插入多条数据：存在重复数据和不重复数据
        // 去除"_id"字段，因为_id是唯一索引
        list.get( 0 ).remove( "_id" );
        cl.insertMany( list );
        // 检查结果
        // 检查个数
        Assert.assertEquals( cl.count( new BasicDBObject() ), num + 2 );
        // 检查内容
        List< Document > actList = cl.find().sort( Sorts.ascending( "a" ) )
                .into( new ArrayList< Document >() );
        for ( int i = 0; i < num + 2; i++ ) {
            if ( i < 3 ) {
                Assert.assertEquals(
                        actList.get( i ).getInteger( "a" ).intValue(),
                        list.get( 0 ).getInteger( "a" ).intValue() );
                Assert.assertEquals(
                        actList.get( i ).getInteger( "b" ).intValue(),
                        list.get( 0 ).getInteger( "b" ).intValue() );
            } else {
                Assert.assertEquals( actList.get( i ), list.get( i - 2 ) );

            }
        }
    }

    @Test
    public void test2() {
        MongoCollection< Document > cl = db.getCollection( clNames[ 1 ] );
        // 创建强制唯一索引
        cl.createIndex( Indexes.ascending( "a" ),
                new IndexOptions().unique( true ).name( "a" ) );
        // 准备记录
        int num = 10;
        List< Document > list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i ).append( "b", i - 1 ) );
        }
        // 插入单条数据
        cl.insertOne( list.get( num / 2 ) );
        // 去除"_id"字段，因为_id是唯一索引
        list.get( num / 2 ).remove( "_id" );
        // 重复插入单条数据
        try {
            cl.insertOne( list.get( num / 2 ) );
            Assert.fail( "exp fail but act success" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -38 ) {
                throw e;
            }
        }
        // 插入多条数据：存在重复数据
        // 去除"_id"字段，因为_id是唯一索引
        list.get( num / 2 ).remove( "_id" );
        try {
            cl.insertMany( list );
            Assert.fail( "exp fail but act success" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -38 ) {
                throw e;
            }
        }
        // 检查结果
        Assert.assertEquals( cl.count( new BasicDBObject() ), num / 2 + 1 );

        // 插入多条数据：不存在重复数据
        cl.insertMany( list.subList( num / 2 + 1, num ) );

        // 检查结果
        // 检查个数
        Assert.assertEquals( cl.count( new BasicDBObject() ), num );
        // 检查内容
        List< Document > actList = cl.find().sort( Sorts.ascending( "a" ) )
                .into( new ArrayList< Document >() );
        for ( int i = 0; i < num; i++ ) {
            Document act = actList.get( i );
            Document exp = list.get( i );
            Assert.assertEquals( act.get( "a" ), exp.get( "a" ),
                    "act = " + act + ",exp = " + exp );
            Assert.assertEquals( act.get( "b" ), exp.get( "b" ),
                    "act = " + act + ",exp = " + exp );

        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clNames );
    }
}
