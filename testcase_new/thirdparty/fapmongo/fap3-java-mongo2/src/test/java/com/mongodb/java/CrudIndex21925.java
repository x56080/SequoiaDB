package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBCursor;
import com.mongodb.DBObject;
import com.mongodb.MongoCommandException;
import com.mongodb.WriteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21925:增删改查索引
 * @author fanyu
 * @Date 2020/3/12
 * @version 1.00
 */

public class CrudIndex21925 extends MongodbTestBase {
    private DB db;
    private String clName = javaDBNameWithVersion + "_cl21925";

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
    }

    @Test
    public void test() {
        // 创建集合
        DBCollection cl = db.createCollection( clName, new BasicDBObject() );
        // 用户未创建索引，列取索引，默认存在$id索引
        List< DBObject > list = cl.getIndexInfo();
        Assert.assertEquals( list.size(), 1, list.toString() );
        Assert.assertEquals( ( ( BSONObject ) list.get( 0 ) ).get( "name" ),
                "$id" );
        Assert.assertEquals( ( ( BSONObject ) list.get( 0 ) ).get( "ns" ),
                db.getName() + "." + clName );

        String[] indexNames1 = { "ascIndex21925", "compoundIndex21925",
                "descIndex21925" };
        String[] indexNames2 = { "$id", "ascIndex21925", "b_1", "c_-1",
                "compoundIndex21925", "descIndex21925", "f_-1_g_1" };

        // 指定索引名，单个字段，创建强制唯一升序索引
        cl.createIndex( new BasicDBObject( "a", 1 ), indexNames1[ 0 ], true );

        // 不指定索引名，单个字段，创建普通升序索引
        cl.createIndex( "b" );

        // 不指定索引名，单个字段，创建强制唯一降序索引
        cl.createIndex( new BasicDBObject( "c", -1 ),
                new BasicDBObject( "unique", true ) );

        // 指定索引名，单个字段，创建普通降序索引
        cl.createIndex( new BasicDBObject( "d", -1 ), indexNames1[ 1 ] );

        // 指定索引名，多个字段，创建强制唯一复合索引
        cl.createIndex( new BasicDBObject( "d", 1 ).append( "e", -1 ),
                indexNames1[ 2 ], true );

        // 不指定索引名，多个字段，创建普通复合索引
        cl.createIndex( new BasicDBObject( "f", -1 ).append( "g", 1 ) );

        // 重复创建索引
        try {
            cl.createIndex( new BasicDBObject( "a", 1 ), indexNames1[ 0 ],
                    true );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-247" ) ) {
                throw e;
            }
        }

        // 做简单的操作
        crud( cl );

        // 列取索引
        List< DBObject > actIndexes = cl.getIndexInfo();
        Assert.assertEquals( actIndexes.size(), indexNames2.length );
        for ( int i = 0; i < actIndexes.size(); i++ ) {
            Assert.assertEquals( actIndexes.get( i ).get( "name" ),
                    indexNames2[ i ] );
        }

        // 删除索引
        for ( int i = 1; i < indexNames2.length; i++ ) {
            cl.dropIndex( indexNames2[ i ] );
        }
        Assert.assertEquals( cl.getIndexInfo().size(), 1 );
    }

    private void crud( DBCollection cl ) {
        List< DBObject > list = new ArrayList<>();
        int num = 10;
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", i + 1 )
                    .append( "c", i + 2 ).append( "d", i + 3 )
                    .append( "e", i + 4 ).append( "f", i + 5 )
                    .append( "g", i + 6 ) );
        }
        // 插入 无重复数据
        cl.insert( list );
        // 插入 有重复数据
        for ( BSONObject bson : list ) {
            bson.removeField( "_id" );
        }
        try {
            cl.insert( list );
            Assert.fail( "exp fail but act success" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -38 ) {
                throw e;
            }
        }
        // 查询
        DBCursor result2 = cl.find().sort( new BasicDBObject( "a", 1 ) );
        int i = 0;
        while ( result2.hasNext() ) {
            DBObject act = result2.next();
            DBObject exp = list.get( i );
            act.removeField( "_id" );
            exp.removeField( "_id" );
            Assert.assertEquals( act.toString(), exp.toString() );
            i++;
        }
        Assert.assertEquals( i, num );

        // 更新 无重复数据
        WriteResult result3 = cl.update( new BasicDBObject( "a", 1 ),
                new BasicDBObject( "$set",
                        new BasicBSONObject( "a", num + 1 ) ) );
        Assert.assertEquals( result3.getN(), 1 );
        Assert.assertEquals( cl.count( new BasicDBObject( "a", num + 1 ) ), 1 );

        // 更新 有重复数据:索引键值重复
        try {
            cl.update( new BasicDBObject( "a", 2 ), new BasicDBObject( "$set",
                    new BasicBSONObject( "a", 3 ).append( "e", 5 ) ) );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }

        // upsert 有重复数据:索引键值重复
        try {
            cl.update( new BasicDBObject( "a", num * 2 ),
                    new BasicDBObject( "$set",
                            new BasicBSONObject( "a", 3 ).append( "e", 5 ) ),
                    true, false );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }

        // 刪除
        WriteResult result5 = cl.remove( new BasicDBObject() );
        Assert.assertEquals( result5.getN(), num );
        Assert.assertEquals( cl.count( new BasicDBObject() ), 0 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
