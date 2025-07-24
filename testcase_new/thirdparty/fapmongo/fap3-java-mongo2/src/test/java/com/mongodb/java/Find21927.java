package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Set;

import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.MongoException;
import com.mongodb.QueryBuilder;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-21927:find操作
 * @author fanyu
 * @Date:2020/3/12
 * @version:1.0
 */
public class Find21927 extends MongodbTestBase {
    private DB db;
    private String clName;
    private DBCollection cl;
    // 不能小于20
    private int num = 20;
    private List< DBObject > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        clName = javaDBNameWithVersion + "_cl21927";

        list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", "" + i )
                    .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
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
        DBObject sort = new BasicDBObject( "a", 1 );
        List< DBObject > actResult;
        // 不带条件查询
        actResult = cl.find().sort( sort ).toArray();
        checkFindResult( actResult, list );
        // 带空条件进行查询
        actResult = cl.find( new BasicDBObject() ).sort( sort ).toArray();
        checkFindResult( actResult, list );
    }

    @Test
    public void test2() {
        DBObject query;
        List< DBObject > actResult;
        // 带匹配符进行查询
        DBObject sort = new BasicDBObject( "a", 1 );
        // and ne lt gt 查询
        query = QueryBuilder.start( "a" ).lessThan( 3 ).greaterThan( -1 )
                .and( "b" ).notEquals( 2 ).get();
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult, list.subList( 0, 3 ) );

        // or lte gte 查询
        query = QueryBuilder.start()
                .or( QueryBuilder.start( "a" ).greaterThanEquals( 5 ).get(),
                        QueryBuilder.start( "a" ).lessThanEquals( 3 ).get() )
                .get();
        actResult = cl.find( query ).sort( sort ).toArray();
        List< DBObject > expList2 = new ArrayList<>();
        expList2.addAll( list.subList( 0, 4 ) );
        expList2.addAll( list.subList( 5, list.size() ) );
        checkFindResult( actResult, expList2 );

        // not et查询
        query = new BasicDBObject( "$not",
                Collections.singletonList( new BasicBSONObject( "a", 0 ) ) );
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult, list.subList( 1, list.size() ) );

        // $nin、$exists查询
        query = QueryBuilder.start( "e" ).in( new int[] { 0, 1 } ).and( "a1" )
                .exists( 0 ).get();
        actResult = cl.find( query ).sort( sort ).toArray();
        List< DBObject > expList4 = new ArrayList<>();
        for ( int i = 0; i < list.size(); i++ ) {
            if ( i % 3 < 2 ) {
                expList4.add( list.get( i ) );
            }
        }
        checkFindResult( actResult, expList4 );

        // $in、$exists查询
        query = QueryBuilder.start( "e" ).notIn( new int[] { 4 } ).and( "a" )
                .exists( 1 ).get();
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult, list );

        // $mode $all
        query = QueryBuilder.start( "e" ).mod( new int[] { 3, 0 } ).and( "c" )
                .all( new int[] { 1, 2, 3 } ).get();
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult,
                Collections.singletonList( list.get( 0 ) ) );

        // $size
        query = new BasicDBObject( "c",
                new BasicBSONObject( "$size", 1 ).append( "$et", 3 ) );
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult, list );

        // $type
        query = new BasicDBObject( "a",
                new BasicDBObject( "$type", 2 ).append( "$et", "int32" ) );
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult, list );

        // $regex
        query = new BasicDBObject( "d",
                new BasicDBObject( "$regex", "^test*" ) );
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult, list );
    }

    @Test
    public void test3() {
        DBObject query;
        List< DBObject > actResult;
        DBObject sort;
        // 带匹配符+sort查询
        // 单个字段正序查询
        sort = new BasicDBObject( "a", 1 );
        query = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThanEquals( 10 ).get();
        actResult = cl.find( query ).sort( sort ).toArray();
        checkFindResult( actResult, list.subList( 0, 11 ) );

        // 单个字段逆序查询
        sort = new BasicDBObject( "a", -1 );
        query = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThanEquals( 10 ).get();
        actResult = cl.find( query ).sort( sort ).toArray();
        List< DBObject > expList2 = new ArrayList<>();
        for ( int i = list.subList( 0, 11 ).size() - 1; i >= 0; i-- ) {
            expList2.add( list.get( i ) );
        }
        checkFindResult( actResult, expList2 );

        // 多个字段正逆序
        sort = new BasicDBObject( "a", 1 ).append( "b", -1 );
        actResult = cl.find().sort( sort ).toArray();
        checkFindResult( actResult, list );

        // 不存在的字段排序
        sort = new BasicDBObject( "a1", 1 );
        actResult = cl.find().sort( sort ).toArray();
        checkFindResult( actResult, list );
    }

    @Test
    public void test4() {
        DBObject query;
        List< DBObject > actResult;
        DBObject sort;
        // 带匹配符、sort、limit、skip查询
        sort = new BasicDBObject( "a", 1 );
        // skip大于等于匹配的记录数
        actResult = cl.find().skip( num ).limit( 1 ).sort( sort ).toArray();
        Assert.assertEquals( actResult.size(), 0 );

        // skip小于匹配数，limit小于匹配数
        query = QueryBuilder.start( "a" ).lessThanEquals( num ).get();
        actResult = cl.find( query ).skip( 1 ).limit( num / 2 ).sort( sort )
                .toArray();
        checkFindResult( actResult, list.subList( 1, num / 2 + 1 ) );

        // skip小于匹配数，limit大于匹配数
        query = QueryBuilder.start( "a" ).lessThanEquals( num ).get();
        actResult = cl.find( query ).skip( 1 ).limit( num * 2 ).sort( sort )
                .toArray();
        checkFindResult( actResult, list.subList( 1, num ) );

        // 匹配的记录数是0，skip limit
        query = QueryBuilder.start( "a" ).greaterThan( num ).get();
        actResult = cl.find( query ).skip( 1 ).limit( num * 2 ).sort( sort )
                .toArray();
        Assert.assertEquals( actResult.size(), 0 );

        // 带匹配符、sort、limit、skip、hint查询
        // 索引存在，hint使用索引名
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        actResult = cl.find( query ).skip( 1 ).limit( num * 2 ).sort( sort )
                .hint( "a" ).toArray();
        checkFindResult( actResult, list.subList( 1, num ) );
        // 索引存在,hint使用键值对的方式
        query = QueryBuilder.start( "e" ).greaterThan( -1 ).get();
        actResult = cl.find( query ).skip( 1 ).limit( num * 2 ).sort( sort )
                .hint( new BasicDBObject( "bf", 1 ) ).toArray();
        checkFindResult( actResult, list.subList( 1, num ) );
        // 索引不存在，hint使用索引名
        query = QueryBuilder.start( "e" ).greaterThan( -1 ).get();
        actResult = cl.find( query ).skip( 1 ).limit( num * 2 ).sort( sort )
                .hint( "inexistence" ).toArray();
        checkFindResult( actResult, list.subList( 1, num ) );
        // 索引不存在，hint使用键值对的方式
        query = QueryBuilder.start( "e" ).greaterThan( -1 ).get();
        actResult = cl.find( query ).skip( 1 ).limit( num * 2 ).sort( sort )
                .hint( "inexistence" ).toArray();
        checkFindResult( actResult, list.subList( 1, num ) );
        // limit为0
        query = QueryBuilder.start( "a" ).lessThanEquals( num ).get();
        actResult = cl.find( query ).limit( 0 ).sort( sort ).toArray();
        Assert.assertEquals( actResult, list );
        // limit为-1
        query = QueryBuilder.start( "a" ).lessThanEquals( num ).get();
        actResult = cl.find( query ).limit( -1 ).sort( sort ).toArray();
        Assert.assertEquals( actResult, list.subList( 0, 1 ) );
    }

    @Test
    public void test5() {
        DBObject query;
        DBObject select;
        List< DBObject > actResult;
        // 带匹配符、sort、limit、skip、hint、选择符查询
        DBObject sort = new BasicDBObject( "a", 1 );
        // 存在的字段,匹配到记录，排除_id
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).lessThanEquals( 10 )
                .get();
        select = new BasicDBObject( "_id", 0 );
        actResult = cl.find( query, select ).skip( 1 ).limit( num / 2 )
                .hint( "a" ).sort( sort ).toArray();
        String[] includeNames1 = new String[] { "a", "b", "c", "d", "e", "f",
                "g" };
        checkFindResult( actResult, list.subList( 1, 11 ), includeNames1 );

        // 存在的字段,匹配不到记录，排除_id
        query = QueryBuilder.start( "a" ).greaterThan( num ).get();
        select = new BasicDBObject( "_id", 1 );
        actResult = cl.find( query, select ).skip( 1 ).limit( num / 2 )
                .sort( sort ).hint( "a" ).toArray();
        Assert.assertEquals( actResult.size(), 0 );

        // 存在字段，排除其它字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).lessThanEquals( 10 )
                .get();
        select = new BasicDBObject( "a", 0 ).append( "b", 0 );
        actResult = cl.find( query, select ).skip( 1 ).limit( num / 2 )
                .hint( "a" ).sort( sort ).toArray();
        String[] includeName9 = new String[] { "c", "d", "e", "f", "g", "_id" };
        checkFindResult( actResult, list.subList( 1, 11 ), includeName9 );

        // 存在字段，选择id字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).lessThanEquals( 10 )
                .get();
        select = new BasicDBObject( "_id", 1 );
        actResult = cl.find( query, select ).skip( 1 ).limit( num / 2 )
                .hint( "a" ).sort( sort ).toArray();
        String[] includeName10 = new String[] { "_id" };
        checkFindResult( actResult, list.subList( 1, 11 ), includeName10 );

        // 存在字段，选择普通字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).lessThanEquals( 10 )
                .get();
        select = new BasicDBObject( "c", 1 ).append( "d", 1 );
        actResult = cl.find( query, select ).skip( 1 ).limit( num / 2 )
                .hint( "a" ).sort( sort ).toArray();
        String[] includeName11 = new String[] { "c", "d", "_id" };
        checkFindResult( actResult, list.subList( 1, 11 ), includeName11 );

        // 排除_id和选择其他字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "a", 1 ).append( "b", 1 ).append( "c", 1 )
                .append( "_id", 0 );
        actResult = cl.find( query, select ).sort( sort ).skip( 2 ).limit( 10 )
                .hint( "bf" ).toArray();
        String[] includeName12 = new String[] { "a", "b", "c" };
        checkFindResult( actResult, list.subList( 2, 12 ), includeName12 );

        // 选择_id和选择其他字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "a", 1 ).append( "b", 1 ).append( "c", 1 )
                .append( "_id", 1 );
        actResult = cl.find( query, select ).sort( sort ).skip( 2 ).limit( 10 )
                .hint( "bf" ).toArray();
        String[] includeName13 = new String[] { "a", "b", "c", "_id" };
        checkFindResult( actResult, list.subList( 2, 12 ), includeName13 );

        // 选择_id和排除其他字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "a", 0 ).append( "b", 0 ).append( "c", 0 )
                .append( "_id", 1 );
        try {
            cl.find( query, select ).sort( sort ).skip( 2 ).limit( 3 )
                    .hint( "bf" ).toArray();
            Assert.fail( "exp fail but act success!!!" );
        } catch ( MongoException e ) {
            if ( e.getCode() != -6 ) {
                throw e;
            }
        }

        // 选择_id和排除其他字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "a", 0 ).append( "b", 0 ).append( "c", 1 );
        try {
            cl.find( query, select ).sort( sort ).skip( 2 ).limit( 3 )
                    .hint( "bf" ).toArray();
            Assert.fail( "exp failed but act success!!!" );
        } catch ( MongoException e ) {
            if ( e.getCode() != -6 ) {
                throw e;
            }
        }

        // 排除所有字段
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "a", 0 ).append( "b", 0 ).append( "c", 0 )
                .append( "d", 0 ).append( "e", 0 ).append( "f", 0 )
                .append( "g", 0 ).append( "_id", 0 );
        actResult = cl.find( query, select ).sort( sort ).skip( 2 ).limit( 10 )
                .hint( "bf" ).toArray();
        Assert.assertEquals( actResult.size(), 10 );
        for ( int i = 0; i < actResult.size(); i++ ) {
            Assert.assertEquals( actResult.get( 0 ).keySet().size(), 0 );
        }

        // 选择的字段不存在
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "k", 1 );
        actResult = cl.find( query, select ).sort( sort ).skip( 2 ).limit( 10 )
                .hint( "bf" ).toArray();
        String[] includeName17 = new String[] { "_id" };
        checkFindResult( actResult, list.subList( 2, 12 ), includeName17 );

        // 排除的字段不存在
        // TODO:已提单
        // DBObject query18 = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        // DBObject select18 = new BasicDBObject( "k", 0 );
        // List< DBObject > result18 = cl.find( query18, select18 )
        // .sort( sort )
        // .skip( 2 )
        // .limit( 10 )
        // .hint( "bf" )
        // .toArray();
        // checkFindResult( result18, list.subList( 1, num ) );

        // 选择字段部分字段存在，部分字段不存在
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "k", 1 ).append( "a", 1 );
        actResult = cl.find( query, select ).sort( sort ).skip( 2 ).limit( 10 )
                .hint( "bf" ).toArray();
        String[] includeName19 = new String[] { "a", "_id" };
        checkFindResult( actResult, list.subList( 2, 12 ), includeName19 );

        // 排除字段部分字段存在，部分字段不存在
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "k", 0 ).append( "a", 0 );
        actResult = cl.find( query, select ).sort( sort ).skip( 2 ).limit( 10 )
                .hint( "bf" ).toArray();
        String[] includeName20 = new String[] { "b", "c", "d", "e", "f", "g",
                "_id" };
        checkFindResult( actResult, list.subList( 2, 12 ), includeName20 );

        // 选择符 eleMatch
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "g", new BasicDBObject( "$elemMatch",
                new BasicDBObject( "a", new BasicDBObject( "$lt", num ) ) ) );
        actResult = cl.find( query, select ).skip( 1 ).limit( num ).sort( sort )
                .hint( "bf" ).toArray();
        checkFindResult( actResult, list.subList( 1, num ) );

        // 选择符 slice
        query = QueryBuilder.start( "a" ).greaterThan( -1 ).get();
        select = new BasicDBObject( "g", new BasicDBObject( "$slice", 2 ) );
        actResult = cl.find( query, select ).skip( 4 ).limit( num ).sort( sort )
                .hint( "bf" ).toArray();
        checkFindResult( actResult, list.subList( 4, num ) );

        // 匹配的记录数为0条
        query = QueryBuilder.start( "a" ).greaterThan( num ).get();
        select = new BasicDBObject( "g", new BasicDBObject( "$slice", 2 ) );
        actResult = cl.find( query, select ).skip( 4 ).limit( num ).sort( sort )
                .hint( "bf" ).toArray();
        Assert.assertEquals( actResult.size(), 0 );

        // $slice 选择的字段不存在
        query = QueryBuilder.start( "a" ).greaterThan( 1 ).get();
        select = new BasicDBObject( "k", new BasicDBObject( "$slice", 2 ) );
        actResult = cl.find( query, select ).skip( 4 ).limit( num ).sort( sort )
                .hint( "bf" ).toArray();
        checkFindResult( actResult, list.subList( 6, num ) );
    }

    @Test
    public void test6() {
        // 带不存在的字段进行查询
        DBObject query1 = QueryBuilder.start( "k" ).greaterThan( -1 )
                .lessThanEquals( 10 ).get();
        List< DBObject > result1 = cl.find( query1 ).toArray();
        Assert.assertEquals( result1.size(), 0 );

        // 集合不存在数据，进行查询
        DBCollection cl1 = db.createCollection( clName + "_6",
                new BasicDBObject() );
        List< DBObject > result2 = cl1.find( query1 ).toArray();
        Assert.assertEquals( result2.size(), 0 );
        cl1.drop();
    }

    @Test
    public void test7() {
        DBObject query;
        DBObject actResult;
        // 查询单个
        actResult = cl.findOne();
        String id = actResult.get( "_id" ).toString();

        checkFindResult( Collections.singletonList( actResult ),
                Collections.singletonList( list.get( 0 ) ) );

        // 使用id进行查询
        actResult = cl.findOne( new ObjectId( id ) );
        checkFindResult( Collections.singletonList( actResult ),
                Collections.singletonList( list.get( 0 ) ) );

        DBObject projection = new BasicDBObject( "_id", 0 );
        actResult = cl.findOne( new ObjectId( id ), projection );
        String[] includeNames1 = new String[] { "a", "b", "c", "d", "e", "f",
                "g" };
        checkFindResult( Collections.singletonList( actResult ),
                Collections.singletonList( list.get( 0 ) ), includeNames1 );

        // 带条件查询一个
        query = QueryBuilder.start( "a" ).greaterThan( 1 ).get();
        actResult = cl.findOne( query );
        checkFindResult( Collections.singletonList( actResult ),
                Collections.singletonList( list.get( 2 ) ) );

        // 带条件+字段查询一个
        query = QueryBuilder.start( "a" ).greaterThan( 1 ).get();
        // 排除a
        DBObject select5 = new BasicDBObject( "a", 0 );
        actResult = cl.findOne( query, select5 );
        DBObject expObject5 = new BasicDBObject( list.get( 2 ).toMap() );
        expObject5.removeField( "a" );
        checkFindResult( Collections.singletonList( actResult ),
                Collections.singletonList( expObject5 ) );

        // 带条件+字段+sort查询一个
        query = QueryBuilder.start( "a" ).greaterThan( 2 ).get();
        DBObject select6 = new BasicDBObject( "a", 0 );
        DBObject sort6 = new BasicDBObject( "a", -1 );
        actResult = cl.findOne( query, select6, sort6 );
        DBObject expObject6 = new BasicDBObject( list.get( num - 1 ).toMap() );
        expObject6.removeField( "a" );
        checkFindResult( Collections.singletonList( actResult ),
                Collections.singletonList( expObject6 ) );
    }

    private void checkFindResult( List< DBObject > actList,
            List< DBObject > expList, String... includeFieldNames ) {
        if ( expList == null || expList.isEmpty() ) {
            Assert.fail( "expList is null or empty" );
        }
        if ( includeFieldNames != null && includeFieldNames.length == 0 ) {
            for ( int i = 0; i < expList.size(); i++ ) {
                DBObject act = actList.get( i );
                DBObject exp = expList.get( i );
                Assert.assertEquals( act, exp );
            }
        } else {
            Set< String > keySet = expList.get( 0 ).keySet();
            Set< String > actKeySet = actList.get( 0 ).keySet();
            Assert.assertTrue( keySet.containsAll( actKeySet ),
                    "expKeySet = " + keySet + ",actKeySet = " + actKeySet );
            List< String > excludeFieldNames = new ArrayList<>();
            excludeFieldNames.addAll( keySet );
            excludeFieldNames.removeAll( Arrays.asList( includeFieldNames ) );
            for ( int i = 0; i < expList.size(); i++ ) {
                DBObject act = actList.get( i );
                DBObject exp = expList.get( i );
                for ( String field : includeFieldNames ) {
                    Assert.assertEquals( act.get( field ), exp.get( field ),
                            "field = " + field );
                }
                for ( String field : excludeFieldNames ) {
                    Assert.assertNull( act.get( field ), "field = " + field );
                }
            }
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
