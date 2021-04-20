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
import com.mongodb.QueryBuilder;
import com.mongodb.WriteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21928:update操作
 * @author fanyu
 * @Date 2020/3/16
 * @version 1.00
 */
public class Update21928 extends MongodbTestBase {
    private DB db;
    private String clName = javaDBNameWithVersion + "_cl21928";
    private DBCollection cl;
    // 不能小于30
    private int num = 30;
    private List< DBObject > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", "" + i )
                    .append( "c", new int[] { i + 1, i + 2, i + 3 } )
                    .append( "d", "test-" + i + "-" + i % 3 )
                    .append( "e", i % 3 ).append( "f", i ).append( "g",
                            Arrays.asList(
                                    new BasicDBObject( "a", i % 3 ).append( "b",
                                            i + 1 ),
                                    new BasicDBObject( "a", i % 4 ).append( "b",
                                            i + num ) ) ) );
        }
        cl = db.getCollection( clName );
        cl.insert( list );
        cl.createIndex( new BasicDBObject( "a", 1 ), "a" );
        cl.createIndex( new BasicDBObject( "b", 1 ).append( "f", 1 ), "bf" );
    }

    @Test
    public void test1() {
        // $inc
        // 单个字段
        DBObject query1 = QueryBuilder.start( "a" ).lessThan( num / 3 ).get();
        DBObject update1 = new BasicDBObject( "$inc",
                new BasicBSONObject( "a", num + 50 ) );
        WriteResult result1 = cl.updateMulti( query1, update1 );
        DBObject check1 = QueryBuilder.start( "a" ).greaterThan( 0 )
                .lessThan( num / 3 ).get();
        Assert.assertTrue( result1.isUpdateOfExisting() );
        Assert.assertEquals( result1.getN(), num / 3 );
        Assert.assertEquals( cl.count( check1 ), 0 );
        Assert.assertEquals( cl.count(
                QueryBuilder.start( "a" ).greaterThanEquals( num + 50 ).get() ),
                num / 3 );

        // 多个字段
        DBObject query2 = QueryBuilder.start( "a" )
                .greaterThanEquals( num + 50 ).get();
        DBObject update2 = new BasicDBObject( "$inc",
                new BasicBSONObject( "a", -( num + 50 ) ).append( "f",
                        num + 50 ) );
        WriteResult result2 = cl.updateMulti( query2, update2 );
        DBObject check2 = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThanEquals( num / 3 ).and( "f" )
                .greaterThanEquals( num + 50 ).get();
        Assert.assertTrue( result2.isUpdateOfExisting() );
        Assert.assertEquals( result2.getN(), num / 3 );
        Assert.assertEquals( cl.count(
                QueryBuilder.start( "a" ).greaterThanEquals( num + 50 ).get() ),
                0 );
        Assert.assertEquals( cl.count( check2 ), num / 3 );

        // 不存在的字段
        DBObject query3 = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThanEquals( num / 3 ).and( "f" )
                .greaterThanEquals( num + 50 ).get();
        DBObject update3 = new BasicDBObject( "$inc",
                new BasicBSONObject( "h", -( num + 50 ) ) );
        WriteResult result3 = cl.updateMulti( query3, update3 );
        DBObject check3 = QueryBuilder.start( "h" ).is( -( num + 50 ) ).get();
        Assert.assertTrue( result3.isUpdateOfExisting() );
        Assert.assertEquals( result3.getN(), num / 3 );
        Assert.assertEquals( cl.count( check3 ), num / 3 );

        // $set
        // 单个字段
        DBObject query4 = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThanEquals( num / 3 ).and( "f" )
                .greaterThanEquals( num + 50 ).get();
        DBObject update4 = new BasicDBObject( "$set",
                new BasicDBObject( "a", 0 ) );
        WriteResult result4 = cl.updateMulti( query4, update4 );
        DBObject check4 = QueryBuilder.start( "a" ).is( 0 ).get();
        Assert.assertTrue( result4.isUpdateOfExisting() );
        Assert.assertEquals( result4.getN(), num / 3 );
        Assert.assertEquals( cl.count( check4 ), num / 3 );

        // 多个字段
        DBObject query5 = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThanEquals( num / 3 ).and( "f" )
                .greaterThanEquals( num + 50 ).get();
        DBObject update5 = new BasicDBObject( "$set",
                new BasicDBObject( "a", 0 ).append( "f", 0 ) );
        WriteResult result5 = cl.updateMulti( query5, update5 );
        DBObject check5 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        Assert.assertTrue( result5.isUpdateOfExisting() );
        Assert.assertEquals( result5.getN(), num / 3 );
        Assert.assertEquals( cl.count( query5 ), 0 );
        Assert.assertEquals( cl.count( check5 ), num / 3 );

        // 数组
        DBObject query6 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update6 = new BasicDBObject( "$set",
                new BasicDBObject( "c.0", 1 ) );
        WriteResult result6 = cl.updateMulti( query6, update6 );
        DBObject check6 = QueryBuilder.start( "c.0" ).is( 1 ).get();
        Assert.assertTrue( result6.isUpdateOfExisting() );
        Assert.assertEquals( result6.getN(), num / 3 );
        Assert.assertEquals( cl.count( query6 ), num / 3 );
        Assert.assertEquals( cl.count( check6 ), num / 3 );

        // 不存在的字段
        DBObject query7 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update7 = new BasicDBObject( "$set",
                new BasicDBObject( "l", 1 ) );
        WriteResult result7 = cl.updateMulti( query7, update7 );
        DBObject check7 = QueryBuilder.start( "l" ).is( 1 ).get();
        Assert.assertTrue( result7.isUpdateOfExisting() );
        Assert.assertEquals( result7.getN(), num / 3 );
        Assert.assertEquals( cl.count( query7 ), num / 3 );
        Assert.assertEquals( cl.count( check7 ), num / 3 );

        // $unset
        // 单个字段
        DBObject query8 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update8 = new BasicDBObject( "$unset",
                new BasicDBObject( "l", 1 ) );
        WriteResult result8 = cl.updateMulti( query8, update8 );
        DBObject check8 = QueryBuilder.start( "l" ).is( 1 ).get();
        Assert.assertTrue( result8.isUpdateOfExisting() );
        Assert.assertEquals( result8.getN(), num / 3 );
        Assert.assertEquals( cl.count( query8 ), num / 3 );
        Assert.assertEquals( cl.count( check8 ), 0 );

        // 数组
        DBObject query9 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update9 = new BasicDBObject( "$unset",
                new BasicDBObject( "c.0", "" ) );
        WriteResult result9 = cl.updateMulti( query9, update9 );
        DBObject check9 = QueryBuilder.start( "c.0" ).is( null ).get();
        Assert.assertTrue( result9.isUpdateOfExisting() );
        Assert.assertEquals( result9.getN(), num / 3 );
        Assert.assertEquals( cl.count( query9 ), num / 3 );
        Assert.assertEquals( cl.count( check9 ), num / 3 );

        // 不存在的字段
        DBObject query10 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update10 = new BasicDBObject( "$unset",
                new BasicDBObject( "k", "" ) );
        WriteResult result10 = cl.updateMulti( query10, update10 );
        DBObject check10 = QueryBuilder.start( "k" ).is( null ).get();
        Assert.assertTrue( result10.isUpdateOfExisting() );
        Assert.assertEquals( result10.getN(), num / 3 );
        Assert.assertEquals( cl.count( query10 ), num / 3 );
        Assert.assertEquals( cl.count( check10 ), 0 );

        // $pop
        // 删除数组中元素
        DBObject query11 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update11 = new BasicDBObject( "$pop",
                new BasicDBObject( "c", -1 ) );
        WriteResult result11 = cl.updateMulti( query11, update11 );
        DBObject check11 = new BasicDBObject( "c",
                new BasicDBObject( "$size", 1 ).append( "$et", 2 ) );
        Assert.assertTrue( result11.isUpdateOfExisting() );
        Assert.assertEquals( result11.getN(), num / 3 );
        Assert.assertEquals( cl.count( query11 ), num / 3 );
        Assert.assertEquals( cl.count( check11 ), num / 3 );

        // 指定的字段不存在
        DBObject query12 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update12 = new BasicDBObject( "$pop",
                new BasicDBObject( "m", -1 ) );
        DBObject check12 = QueryBuilder.start( "m" ).exists( 0 ).and( "a" )
                .is( 0 ).get();
        WriteResult result12 = cl.updateMulti( query12, update12 );
        Assert.assertTrue( result12.isUpdateOfExisting() );
        Assert.assertEquals( result12.getN(), num / 3 );
        Assert.assertEquals( cl.count( query12 ), num / 3 );
        Assert.assertEquals( cl.count( check12 ), num / 3 );

        // $pull
        // 删除数组中的元素
        DBObject query13 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update13 = new BasicDBObject( "$pull",
                new BasicDBObject( "c", 2 ) );
        DBObject check13 = QueryBuilder.start( "c.0" ).is( 2 ).get();
        WriteResult result13 = cl.updateMulti( query13, update13 );
        Assert.assertTrue( result13.isUpdateOfExisting() );
        Assert.assertEquals( result13.getN(), num / 3 );
        Assert.assertEquals( cl.count( query13 ), num / 3 );
        Assert.assertEquals( cl.count( check13 ), 0 );

        // 指定的值不存在
        DBObject query14 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update14 = new BasicDBObject( "$pull",
                new BasicDBObject( "c", num + 1 ) );
        WriteResult result14 = cl.updateMulti( query14, update14 );
        DBObject check14 = QueryBuilder.start( "c.0" ).is( num + 1 ).get();
        Assert.assertTrue( result14.isUpdateOfExisting() );
        Assert.assertEquals( result14.getN(), num / 3 );
        Assert.assertEquals( cl.count( query14 ), num / 3 );
        Assert.assertEquals( cl.count( check14 ), 0 );

        // $push
        // 向数组中添加元素
        DBObject query15 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update15 = new BasicDBObject( "$push",
                new BasicDBObject( "c", 1 ) );
        WriteResult result15 = cl.updateMulti( query15, update15 );
        DBObject check15 = QueryBuilder.start( "c" ).in( new int[] { 1 } )
                .get();
        Assert.assertTrue( result15.isUpdateOfExisting() );
        Assert.assertEquals( result15.getN(), num / 3 );
        Assert.assertEquals( cl.count( query15 ), num / 3 );
        Assert.assertEquals( cl.count( check15 ), num / 3 );

        // 指定的字段名不存在
        DBObject query16 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update16 = new BasicDBObject( "$push",
                new BasicDBObject( "i", 1 ) );
        WriteResult result16 = cl.updateMulti( query16, update16 );
        DBObject check16 = QueryBuilder.start( "i" ).in( new int[] { 1 } )
                .get();
        Assert.assertTrue( result16.isUpdateOfExisting() );
        Assert.assertEquals( result16.getN(), num / 3 );
        Assert.assertEquals( cl.count( query16 ), num / 3 );
        Assert.assertEquals( cl.count( check16 ), num / 3 );

        // $addtoset
        // 字段存在，向数组中添加元素
        DBObject query17 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update17 = new BasicDBObject( "$addtoset",
                new BasicDBObject( "c", new int[] { 3, 4, 5 } ) );
        WriteResult result17 = cl.updateMulti( query17, update17 );
        DBObject check17 = QueryBuilder.start( "c" ).in( new int[] { 3, 4, 5 } )
                .get();
        Assert.assertTrue( result17.isUpdateOfExisting() );
        Assert.assertEquals( result17.getN(), num / 3 );
        Assert.assertEquals( cl.count( query17 ), num / 3 );
        Assert.assertEquals( cl.count( check17 ), num / 3 );

        // 字段不存在，添加元素
        DBObject query18 = QueryBuilder.start( "a" ).is( 0 ).and( "f" ).is( 0 )
                .get();
        DBObject update18 = new BasicDBObject( "$addtoset",
                new BasicDBObject( "j", new int[] { 3, 4, 5 } ) );
        WriteResult result18 = cl.updateMulti( query18, update18 );
        DBObject check18 = QueryBuilder.start( "j" ).in( new int[] { 3, 4, 5 } )
                .get();
        Assert.assertTrue( result18.isUpdateOfExisting() );
        Assert.assertEquals( result18.getN(), num / 3 );
        Assert.assertEquals( cl.count( query18 ), num / 3 );
        Assert.assertEquals( cl.count( check18 ), num / 3 );
    }

    @Test
    public void test2() {
        // 更新单条记录
        DBObject query1 = QueryBuilder.start( "a" ).is( 2 * num / 3 + 1 ).get();
        DBObject update1 = new BasicDBObject( "a", 2 * num / 3 + 1 - 100 );
        WriteResult result1 = cl.update( query1, update1 );
        DBObject check1 = QueryBuilder.start( "a" ).is( 2 * num / 3 + 1 - 100 )
                .get();
        Assert.assertTrue( result1.isUpdateOfExisting() );
        Assert.assertEquals( result1.getN(), 1 );
        Assert.assertEquals( cl.count( query1 ), 0 );
        Assert.assertEquals( cl.count( check1 ), 1 );

        // 匹配不到记录，更新单条记录
        DBObject query2 = QueryBuilder.start( "a" ).is( 2 * num / 3 + 100 )
                .get();
        DBObject update2 = new BasicDBObject( "$set",
                new BasicDBObject( "a", 2 * num / 3 + 1 - 100 ) );
        WriteResult result2 = cl.update( query2, update2 );
        DBObject check2 = QueryBuilder.start( "a" ).is( 2 * num / 3 + 1 - 200 )
                .get();
        Assert.assertFalse( result2.isUpdateOfExisting() );
        Assert.assertEquals( result2.getN(), 0 );
        Assert.assertEquals( cl.count( query2 ), 0 );
        Assert.assertEquals( cl.count( check2 ), 0 );

        // 匹配到记录，更新的字段不存在
        DBObject query3 = QueryBuilder.start( "a" ).is( 2 * num / 3 + 2 ).get();
        DBObject update3 = new BasicDBObject( "$set",
                new BasicDBObject( "x", 2 * num / 3 + 1 - 100 ) );
        WriteResult result3 = cl.update( query3, update3 );
        DBObject check3 = QueryBuilder.start( "x" ).is( 2 * num / 3 + 1 - 100 )
                .get();
        Assert.assertTrue( result3.isUpdateOfExisting() );
        Assert.assertEquals( result3.getN(), 1 );
        Assert.assertEquals( cl.count( query3 ), 1 );
        Assert.assertEquals( cl.count( check3 ), 1 );

        // 匹配不到记录，更新多条记录
        DBObject query4 = QueryBuilder.start( "a" ).is( 2 * num / 3 + 100 )
                .get();
        DBObject update4 = new BasicDBObject( "$set",
                new BasicDBObject( "a", 2 * num / 3 + 1 - 100 ) );
        WriteResult result4 = cl.updateMulti( query4, update4 );
        DBObject check4 = QueryBuilder.start( "a" ).is( 2 * num / 3 + 1 - 200 )
                .get();
        Assert.assertFalse( result4.isUpdateOfExisting() );
        Assert.assertEquals( result4.getN(), 0 );
        Assert.assertEquals( cl.count( query2 ), 0 );
        Assert.assertEquals( cl.count( check4 ), 0 );

        // 集合不存在记录，更新记录
        if ( db.collectionExists( clName + "-test2" ) ) {
            db.getCollection( clName + "-test2" ).drop();
        }
        DBCollection cl1 = db.createCollection( clName + "-test2",
                new BasicDBObject() );
        WriteResult result5 = cl1.updateMulti( new BasicDBObject(),
                new BasicDBObject( "$set", new BasicDBObject( "a", 6 ) ) );
        Assert.assertEquals( result5.isUpdateOfExisting(), false );
        Assert.assertEquals( result5.getN(), 0 );
        Assert.assertEquals( cl1.count(), 0 );
    }

    @Test
    public void test3() {
        // 设置upsert更新记录
        // 不存在记录，upsert为false更新记录
        DBObject query1 = QueryBuilder.start( "a" ).is( num + 1 ).get();
        DBObject update1 = new BasicDBObject( "a", num - 100 );
        WriteResult result1 = cl.update( query1, update1, false, false );
        DBObject check1 = QueryBuilder.start( "a" ).is( num - 100 ).get();
        Assert.assertFalse( result1.isUpdateOfExisting() );
        Assert.assertEquals( result1.getN(), 0 );
        Assert.assertEquals( cl.count( query1 ), 0 );
        Assert.assertEquals( cl.count( check1 ), 0 );

        // 存在记录，upsert为false更新记录
        DBObject query2 = QueryBuilder.start( "a" ).is( num - 1 ).get();
        DBObject update2 = new BasicDBObject( "a", num - 100 );
        WriteResult result2 = cl.update( query2, update2, false, false );
        DBObject check2 = QueryBuilder.start( "a" ).is( num - 100 ).get();
        Assert.assertTrue( result2.isUpdateOfExisting() );
        Assert.assertEquals( result2.getN(), 1 );
        Assert.assertEquals( cl.count( query2 ), 0 );
        Assert.assertEquals( cl.count( check2 ), 1 );

        // 不存在记录，upsert为true更新记录
        DBObject query3 = QueryBuilder.start( "a" ).is( num - 1 ).get();
        DBObject update3 = new BasicDBObject( "a", num - 200 );
        WriteResult result3 = cl.update( query3, update3, true, false );
        DBObject check3 = QueryBuilder.start( "a" ).is( num - 200 ).get();
        Assert.assertFalse( result3.isUpdateOfExisting() );
        Assert.assertEquals( result3.getN(), 1 );
        Assert.assertEquals( cl.count( query3 ), 0 );
        Assert.assertEquals( cl.count( check3 ), 1 );

        // 存在记录，upsert为true更新记录
        DBObject query4 = QueryBuilder.start( "a" ).is( num - 3 ).get();
        DBObject update4 = new BasicDBObject( "a", num - 300 );

        WriteResult result4 = cl.update( query4, update4, true, false );
        DBObject check4 = QueryBuilder.start( "a" ).is( num - 300 ).get();
        Assert.assertTrue( result4.isUpdateOfExisting() );
        Assert.assertEquals( result4.getN(), 1 );
        Assert.assertEquals( cl.count( query4 ), 0 );
        Assert.assertEquals( cl.count( check4 ), 1 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
