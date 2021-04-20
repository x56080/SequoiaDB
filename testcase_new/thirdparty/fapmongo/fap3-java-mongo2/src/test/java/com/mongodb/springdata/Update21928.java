package com.mongodb.springdata;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.core.query.Update;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.WriteResult;
import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21928:update操作
 * @author fanyu
 * @Date 2020/3/20
 * @version 1.00
 */
public class Update21928 extends MongodbTestBase {
    private String[] clNames = { springDBNameWithVersion + "_cl21928_1",
            springDBNameWithVersion + "_cl21928_2",
            springDBNameWithVersion + "_cl21928_3",
            springDBNameWithVersion + "_cl21928_4" };
    // 不能小于30
    private int num = 30;

    @BeforeClass
    public void setUp() {
    }

    @Test
    public void test1() {
        Query query;
        Query check;
        Update update;
        WriteResult result;
        String clName = clNames[ 0 ];
        prepare( clName );
        // $inc
        // 单个字段
        query = new Query( Criteria.where( "age" ).lt( num / 7 ) );
        update = new Update().inc( "age", num * 3 );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "age" ).gte( num * 3 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );
        Assert.assertEquals( mongoTemplate.count( check, clName ), num / 7 );

        // 多个字段
        query = new Query( Criteria.where( "age" ).gte( num * 3 )
                .lt( num / 7 + num * 3 ) );
        update = new Update().inc( "age", -num * 3 ).inc( "grade", 1 );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "age" ).lt( num / 7 ).and( "grade" )
                .lt( num / 7 + 1 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );
        Assert.assertEquals( mongoTemplate.count( check, clName ), num / 7 );

        // 不存在的字段
        query = new Query( Criteria.where( "age" ).lt( num / 7 ) );
        update = new Update().inc( "k", -num * 3 );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query(
                Criteria.where( "age" ).lt( num / 7 ).and( "k" ).lt( 0 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), num / 7 );
        Assert.assertEquals( mongoTemplate.count( check, clName ), num / 7 );

        // $set
        // 单个字段
        query = new Query(
                Criteria.where( "age" ).gte( num / 7 ).lt( 2 * num / 7 ) );
        update = new Update().set( "age", num / 7 );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "age" ).is( num / 7 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), num / 7 );
        Assert.assertEquals( mongoTemplate.count( check, clName ), num / 7 );

        // 多个字段
        query = new Query( Criteria.where( "age" ).is( num / 7 ) );
        update = new Update().set( "age", num / 7 + num / 8 ).set( "grade",
                num / 7 + num / 8 );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "age" ).is( num / 7 + num / 8 )
                .and( "grade" ).is( num / 7 + num / 8 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );
        Assert.assertEquals( mongoTemplate.count( check, clName ), num / 7 );

        // $unset
        // 单个字段
        query = new Query(
                Criteria.where( "age" ).gte( 2 * num / 7 ).lt( 3 * num / 7 ) );
        update = new Update().unset( "grade" );
        result = mongoTemplate.updateMulti( query, update, clName );
        Query check8A = new Query( Criteria.where( "age" ).gte( 2 * num / 7 )
                .lt( 3 * num / 7 ).and( "grade" ).exists( false ) );
        Query check8B = new Query( Criteria.where( "age" ).gte( 2 * num / 7 )
                .lt( 3 * num / 7 ).and( "grade" ).exists( true ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( check8B, clName ), 0 );
        Assert.assertEquals( mongoTemplate.count( check8A, clName ), num / 7 );

        // 数组
        query = new Query( Criteria.where( "age" ).gte( 2 * num / 7 )
                .lt( 3 * num / 7 ).and( "courses.0" ).ne( null ) );
        update = new Update().unset( "courses.0" );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "age" ).gte( 2 * num / 7 )
                .lt( 3 * num / 7 ).and( "courses.0" ).is( null ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );
        Assert.assertEquals( mongoTemplate.count( check, clName ), num / 7 );

        // 不存在的字段
        query = new Query(
                Criteria.where( "age" ).gte( 2 * num / 7 ).lt( 3 * num / 7 ) );
        update = new Update().unset( "k" );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "k" ).exists( true ).and( "age" )
                .gte( 2 * num / 7 ).lt( 3 * num / 7 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), num / 7 );
        Assert.assertEquals( mongoTemplate.count( check, clName ), 0 );

        // $pop
        // 删除数组中元素
        query = new Query(
                Criteria.where( "age" ).gte( 3 * num / 7 ).lt( 4 * num / 7 ) );
        update = new Update().pop( "courses", Update.Position.FIRST );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "courses" )
                .all( Entity.COURSES[ 1 ], Entity.COURSES[ 2 ] ).and( "age" )
                .gte( 3 * num / 7 ).lt( 4 * num / 7 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), 4 * num / 7 - 3 * num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ),
                4 * num / 7 - 3 * num / 7 );
        Assert.assertEquals( mongoTemplate.count( check, clName ),
                4 * num / 7 - 3 * num / 7 );

        // 指定的字段不存在
        query = new Query(
                Criteria.where( "age" ).gte( 3 * num / 7 ).lt( 4 * num / 7 ) );
        update = new Update().pop( "k", Update.Position.LAST );
        result = mongoTemplate.updateMulti( query, update, clName );
        check = new Query( Criteria.where( "k" ).exists( false ).and( "age" )
                .gte( 3 * num / 7 ).lt( 4 * num / 7 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), 4 * num / 7 - 3 * num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ),
                4 * num / 7 - 3 * num / 7 );
        Assert.assertEquals( mongoTemplate.count( check, clName ),
                4 * num / 7 - 3 * num / 7 );

        // $pull
        // 删除数组中的元素
        query = new Query(
                Criteria.where( "age" ).gte( 4 * num / 7 ).lt( 5 * num / 7 ) );
        update = new Update().pull( "courses", "english" );
        result = mongoTemplate.updateMulti( query, update, clName );
        Query check13A = new Query(
                Criteria.where( "courses" ).all( Entity.COURSES.toString() )
                        .and( "age" ).gte( 4 * num / 7 ).lt( 5 * num / 7 ) );
        Query check13B = new Query( Criteria.where( "courses" )
                .all( Entity.COURSES[ 0 ], Entity.COURSES[ 1 ] ).and( "age" )
                .gte( 4 * num / 7 ).lt( 5 * num / 7 ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( check13A, clName ), 0 );
        Assert.assertEquals( mongoTemplate.count( check13B, clName ), num / 7 );

        // 指定的值不存在
        query = new Query(
                Criteria.where( "age" ).gte( 4 * num / 7 ).lt( 5 * num / 7 ) );
        update = new Update().pull( "courses", "english1" );
        result = mongoTemplate.updateMulti( query, update, clName );
        Query check14 = new Query( Criteria.where( "age" ).gte( 4 * num / 7 )
                .lt( 5 * num / 7 ).and( "courses" ).nin( "english1" ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), 5 * num / 7 - 4 * num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ),
                5 * num / 7 - 4 * num / 7 );
        Assert.assertEquals( mongoTemplate.count( check14, clName ),
                5 * num / 7 - 4 * num / 7 );

        // $push
        // 向数组中添加元素
        query = new Query(
                Criteria.where( "age" ).gte( 5 * num / 7 ).lt( 6 * num / 7 ) );
        update = new Update().push( "courses", "physical" );
        result = mongoTemplate.updateMulti( query, update, clName );
        Query check15A = new Query(
                Criteria.where( "age" ).gte( 5 * num / 7 ).lt( 6 * num / 7 )
                        .and( "courses" ).in( Entity.COURSES, "physical" ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), 6 * num / 7 - 5 * num / 7 );
        Assert.assertEquals( mongoTemplate.count( check15A, clName ),
                6 * num / 7 - 5 * num / 7 );

        // 指定的字段名不存在
        query = new Query(
                Criteria.where( "age" ).gte( 5 * num / 7 ).lt( 6 * num / 7 ) );
        update = new Update().push( "courses1", "physical" );
        result = mongoTemplate.updateMulti( query, update, clName );
        Query check16 = new Query( Criteria.where( "age" ).gte( 5 * num / 7 )
                .lt( 6 * num / 7 ).and( "courses1" ).exists( true ) );
        Assert.assertTrue( result.isUpdateOfExisting() );
        Assert.assertEquals( result.getN(), num / 7 );
        Assert.assertEquals( mongoTemplate.count( query, clName ), num / 7 );
        Assert.assertEquals( mongoTemplate.count( check16, clName ), num / 7 );

        // $addtoset 操作符addToSet与sequoiadb的操作符不一样
        // 字段存在，向数组中添加元素
        // sequoiadb会报参数错误 跟开发确认暂时不处理
    }

    @Test
    public void test2() {
        String clName = clNames[ 1 ];
        // 集合不存在记录，更新记录
        if ( mongoTemplate.collectionExists( clName ) ) {
            mongoTemplate.getCollection( clName ).drop();
        }
        mongoTemplate.createCollection( clName );
        WriteResult result5 = mongoTemplate.updateMulti( new Query(),
                new Update().set( "a", 0 ), clName );
        Assert.assertEquals( result5.isUpdateOfExisting(), false );
        Assert.assertEquals( result5.getN(), 0 );
        Assert.assertEquals( mongoTemplate.count( new Query(), clName ), 0 );
    }

    @DataProvider(name = "data-provider")
    private Object[][] rangeData() {
        String clName = clNames[ 2 ];
        prepare( clName );
        AtomicInteger count = new AtomicInteger( num );
        int updateBaseValue = 10 * num;
        // int n, boolean updateOfExisting, Object upsertedId
        return new Object[][] {
                // 匹配不到记录，upsert为true,更新普通字段， 不带setOnInsert
                { new Query(
                        Criteria.where( "age" ).is( count.getAndIncrement() ) ),
                        new Update().set( "age", updateBaseValue ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", null ),
                        new Query(
                                Criteria.where( "age" ).is( updateBaseValue ) ),
                        1 },

                // 匹配不到记录，upsert为true,更新_id字段 ,不带setOnInsert
                { new Query( Criteria.where( "age" ).is( count.get() ) ),
                        new Update().set( "_id", 2 * updateBaseValue ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", 2 * updateBaseValue ),
                        new Query( Criteria.where( "age" )
                                .is( count.getAndIncrement() ) ),
                        1 },

                // 匹配不到记录，upsert为true,更新混合字段 不带setOnInsert
                { new Query( Criteria.where( "age" ).is( count.get() ) ),
                        new Update().set( "_id", 3 * updateBaseValue )
                                .set( "age", count.get() ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", 3 * updateBaseValue ),
                        new Query( Criteria.where( "age" )
                                .is( count.getAndIncrement() ) ),
                        1 },

                // 带setOnInsert更新符
                // 匹配不到记录，upsert为true,更新普通字段、查询条件字段相同,带setOnInsert
                { new Query(
                        Criteria.where( "age" ).is( count.getAndIncrement() ) ),
                        new Update().setOnInsert( "age", 4 * updateBaseValue ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", null ),
                        new Query( Criteria.where( "age" )
                                .is( 4 * updateBaseValue ) ),
                        1 },
                // 匹配不到记录，upsert为true,更新_id字段、查询条件字段不同,带setOnInsert
                { new Query( Criteria.where( "age" ).is( count.get() ) ),
                        new Update().setOnInsert( "_id", 5 * updateBaseValue ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", null ),
                        new Query( Criteria.where( "age" )
                                .is( count.getAndIncrement() ) ),
                        1 },
                // 匹配不到记录，upsert为true，混合字段、查询条件字段不同,带setOnInsert
                { new Query( Criteria.where( "age" ).is( count.get() ) ),
                        new Update().setOnInsert( "_id", 6 * updateBaseValue )
                                .set( "age", count.get() ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", 6 * updateBaseValue ),
                        new Query( Criteria.where( "age" )
                                .is( count.getAndIncrement() ) ),
                        1 },
                // 匹配不到记录，upsert为true，普通字段、查询条件字段不同,带setOnInsert + $set
                { new Query( Criteria.where( "age" ).is( count.get() ) ),
                        new Update().setOnInsert( "sex", "w" ).set( "_id",
                                7 * updateBaseValue ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", 7 * updateBaseValue ),
                        new Query( Criteria.where( "age" )
                                .is( count.getAndIncrement() ).and( "sex" )
                                .is( "w" ) ),
                        1 },
                // 匹配不到记录，upsert为true,混合字段、查询条件字段相同,带setOnInsert + $inc
                { new Query( Criteria.where( "age" ).is( count.get() ) ),
                        new Update().setOnInsert( "_id", 8 * updateBaseValue )
                                .inc( "age", 1 ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", 8 * updateBaseValue ),
                        new Query( Criteria.where( "age" )
                                .is( count.getAndIncrement() + 1 ) ),
                        1 },
                // 匹配不到记录，upsert为true,混合字段、查询条件字段不同,带setOnInsert + $unset
                { new Query(
                        Criteria.where( "age" ).is( count.incrementAndGet() ) ),
                        new Update().setOnInsert( "_id", 9 * updateBaseValue )
                                .setOnInsert( "sex", "w" ).unset( "age" ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", false )
                                .append( "upsertId", 9 * updateBaseValue ),
                        new Query( Criteria.where( "sex" ).is( "w" ) ), 1 },
                // 匹配到记录，upsert为true,更新条件带setOnInsert和带其他更新符，更新条件中字段值与更新之前不相等,更新记录
                { new Query( Criteria.where( "age" ).is( num / 2 ) ),
                        new Update().setOnInsert( "_id", 10 * updateBaseValue )
                                .set( "age", 10 * updateBaseValue ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", true )
                                .append( "upsertId", null ),
                        new Query( Criteria.where( "age" )
                                .is( 10 * updateBaseValue ) ),
                        1 },

                // 匹配到记录，upsert为true,更新条件带setOnInsert和带其他更新符，更新条件中字段值与更新之前相等,更新记录
                { new Query( Criteria.where( "age" ).is( num / 3 ) ),
                        new Update().setOnInsert( "_id", 9 * updateBaseValue )
                                .set( "age", count.get() ),
                        new BasicDBObject( "N", 1 )
                                .append( "isUpdateOfExisting", true )
                                .append( "upsertId", null ),
                        new Query( Criteria.where( "age" ).is( count.get() ) ),
                        1 } };
    }

    @Test(dataProvider = "data-provider")
    public void test( Query query, Update update, BasicDBObject expWriteResult,
            Query check, int expCount ) throws Exception {
        try {
            WriteResult actWriteResult = mongoTemplate.upsert( query, update,
                    Entity.class, clNames[ 2 ] );
            Assert.assertEquals( actWriteResult.isUpdateOfExisting(),
                    expWriteResult.getBoolean( "isUpdateOfExisting" ) );
            Assert.assertEquals( actWriteResult.getN(),
                    expWriteResult.get( "N" ) );
            if ( !expWriteResult.getBoolean( "isUpdateOfExisting" ) ) {
                if ( expWriteResult.get( "upsertId" ) == null ) {
                    check.addCriteria( Criteria.where( "_id" )
                            .is( actWriteResult.getUpsertedId() ) );
                } else {
                    Assert.assertEquals( actWriteResult.getUpsertedId(),
                            expWriteResult.get( "upsertId" ) );
                    check.addCriteria( Criteria.where( "_id" )
                            .is( expWriteResult.get( "upsertId" ) ) );
                }
            }
            Assert.assertEquals( mongoTemplate.count( check, clNames[ 2 ] ),
                    expCount );
        } catch ( AssertionError e ) {
            throw new Exception( "query = " + query + "\nupdate = "
                    + update.getUpdateObject().toString(), e );
        }
    }

    @Test
    public void test3() {
        String clName = clNames[ 3 ];
        prepare( clName );
        // 设置upsert更新记录
        // 存在记录，upsert更新记录
        Query query2 = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        Update update2 = new Update().set( "age", num * 3 );
        WriteResult result2 = mongoTemplate.upsert( query2, update2,
                Entity.class, clName );
        Query check2 = new Query( Criteria.where( "age" ).is( num * 3 ) );
        Assert.assertTrue( result2.isUpdateOfExisting() );
        Assert.assertEquals( result2.getN(), 1 );
        Assert.assertEquals( mongoTemplate.count( query2, clName ),
                num / 3 - 1 );
        Assert.assertEquals( mongoTemplate.count( check2, clName ), 1 );

        // 匹配不到记录，upsert更新记录
        Query query3 = new Query( Criteria.where( "age" ).gte( num * 10 ) );
        Update update3 = new Update().set( "age", num * 10 );
        WriteResult result3 = mongoTemplate.upsert( query3, update3, clName );
        Query check3 = new Query( Criteria.where( "age" ).is( num * 10 ) );
        Assert.assertFalse( result3.isUpdateOfExisting() );
        Assert.assertEquals( result3.getN(), 1 );
        Assert.assertEquals( mongoTemplate.count( check3, clName ), 1 );
    }

    private void prepare( String clName ) {
        List< Entity > list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add(
                    new Entity( "a" + i, Entity.SEXS[ i % Entity.SEXS.length ],
                            i, i, Entity.COURSES ) );
        }
        mongoTemplate.insert( list, clName );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clNames );
    }
}
