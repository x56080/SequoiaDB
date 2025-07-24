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
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21926:insert操作
 * @author fanyu
 * @Date 2020/3/12
 * @version 1.00
 */
public class Insert21926 extends MongodbTestBase {
    private DB db;
    private String[] clNames = new String[ 2 ];

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        clNames[ 0 ] = javaDBNameWithVersion + "_cl21926A";
        clNames[ 1 ] = javaDBNameWithVersion + "_cl21926B";
    }

    @Test
    public void test1() {
        DBCollection cl = db.getCollection( clNames[ 0 ] );
        // 创建普通索引
        cl.createIndex( "a" );
        // 准备记录
        int num = 10;
        List< DBObject > list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", i - 1 ) );
        }
        // 插入单条数据
        cl.insert( list.get( 0 ) );
        // 去除"_id"字段，因为_id是唯一索引
        list.get( 0 ).removeField( "_id" );
        // 重复插入单条数据
        cl.insert( list.get( 0 ) );
        // 插入多条数据：存在重复数据和不重复数据
        // 去除"_id"字段，因为_id是唯一索引
        list.get( 0 ).removeField( "_id" );
        cl.insert( list );
        // 检查结果
        Assert.assertEquals( cl.count( new BasicDBObject() ), num + 2 );
        // 检查内容
        List< DBObject > actList = cl.find().sort( new BasicDBObject( "a", 1 ) )
                .toArray();
        for ( int i = 0; i < num + 2; i++ ) {
            if ( i < 3 ) {
                Assert.assertEquals( actList.get( i ).get( "a" ),
                        list.get( 0 ).get( "a" ) );
                Assert.assertEquals( actList.get( i ).get( "b" ),
                        list.get( 0 ).get( "b" ) );
            } else {
                Assert.assertEquals( actList.get( i ), list.get( i - 2 ) );

            }
        }
    }

    @Test
    public void test2() {
        DBCollection cl = db.getCollection( clNames[ 1 ] );
        // 创建强制唯一索引
        cl.createIndex( new BasicDBObject( "a", 1 ), "index", true );
        // 准备记录
        int num = 10;
        List< DBObject > list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", i - 1 ) );
        }
        // 插入单条数据
        cl.insert( list.get( num / 2 ) );
        // 重复插入单条数据
        try {
            cl.insert( list.get( num / 2 ) );
            Assert.fail( "exp fail but act success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }
        // 插入多条数据：存在重复数据
        try {
            cl.insert( list );
            Assert.fail( "exp fail but act success" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }
        // 检查结果
        Assert.assertEquals( cl.count( new BasicDBObject() ), num / 2 + 1 );

        // 插入多条数据：不存在重复数据
        cl.insert( list.subList( num / 2 + 1, num ) );
        Assert.assertEquals( cl.count( new BasicDBObject() ), num );

        // 检查内容
        // 检查内容
        List< DBObject > actList = cl.find().sort( new BasicDBObject( "a", 1 ) )
                .toArray();
        for ( int i = 0; i < num; i++ ) {
            DBObject act = actList.get( i );
            DBObject exp = list.get( i );
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
