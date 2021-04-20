package com.mongodb.java;

import static com.mongodb.client.model.Filters.and;
import static com.mongodb.client.model.Filters.eq;
import static com.mongodb.client.model.Filters.gt;
import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;
import static com.mongodb.client.model.Projections.exclude;
import static com.mongodb.client.model.Projections.excludeId;
import static com.mongodb.client.model.Projections.fields;
import static com.mongodb.client.model.Projections.include;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Set;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.MongoQueryException;
import com.mongodb.client.FindIterable;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Sorts;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-21927:find操作 mongodb driver v3.2至v3.4
 *               find操作无hint相关接口，v3.5有hint​(@Nullable Bson
 *               hint)接口，v3.12有hintString​(@Nullable String hint)接口，
 *               目前fap3只支持到3.7，因此在v3.2至v3.7上暂时不测试hint相关接口，测试fap4时再进行测试
 * @author fanyu
 * @Date:2020/3/22
 * @version:1.0
 */
public class Find21927 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName = javaDBNameWithVersion + "_cl21927";
    private MongoCollection< Document > cl;
    // 3的倍数，不能小于10
    private int num = 18;
    private List< Document > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i ).append( "b", "" + i )
                    .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
                    .append( "d", "test-" + i + "-" + i % 3 )
                    .append( "e", new Document( "a", 1 ).append( "b", 1 ) ) );
        }
        cl = db.getCollection( clName );
        cl.insertMany( list );
    }

    @Test
    public void test1() {
        FindIterable< Document > actFindIterable;
        List< Document > actList;

        // 不带条件查询
        actFindIterable = cl.find().sort( new Document( "a", 1 ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Collections.sort( actList, new Comparator< Document >() {
            @Override
            public int compare( Document o1, Document o2 ) {
                return o1.getInteger( "a" ) - o2.getInteger( "a" );
            }
        } );
        Assert.assertEquals( actList, list );

        // 带空Document对象查询
        actFindIterable = cl.find( new Document() );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Collections.sort( actList, new Comparator< Document >() {
            @Override
            public int compare( Document o1, Document o2 ) {
                return o1.getInteger( "a" ) - o2.getInteger( "a" );
            }
        } );
        Assert.assertEquals( actList, list );

        // 带简单条件查询
        actFindIterable = cl.find( eq( "a", 0 ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 1, actList.toString() );
        Assert.assertEquals( actList.get( 0 ), list.get( 0 ) );

        // 带 and gte lt eq条件查询
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num / 3 ) ) )
                .sort( Sorts.ascending( "a" ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 1, num / 3 ) );

        // 带 and gte lt eq条件查询 + projection选择符
        actFindIterable = cl
                .find( and( gte( "a", 2 ), lt( "a", 2 * num / 3 ) ) )
                .projection( new Document( "a", 1 ).append( "b", 1 )
                        .append( "c", 1 ).append( "d", 1 ).append( "e", 1 )
                        .append( "f", 1 ).append( "g", 1 ) );
        actList = actFindIterable.sort( Sorts.ascending( "a" ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 2, 2 * num / 3 ) );

        // //带sort+projection查询
        actFindIterable = cl.find( and( gte( "a", 3 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( include( "a", "c", "d" ) ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        List< Document > expList6 = list.subList( 3, num );
        for ( int i = 0; i < expList6.size(); i++ ) {
            Document act = actList.get( i );
            Document exp = expList6.get( i );
            Assert.assertEquals( act.get( "a" ), exp.get( "a" ) );
            Assert.assertEquals( act.get( "c" ), exp.get( "c" ) );
            Assert.assertEquals( act.get( "d" ), exp.get( "d" ) );
            Assert.assertEquals( act.get( "_id" ), exp.get( "_id" ) );
            Assert.assertNull( act.get( "b" ) );
        }

        // //带sort+limit+skip+projection查询
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.descending( "a" ) )
                .projection( fields( include( "a", "b", "c" ) ) ).skip( 1 )
                .limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        List< Document > expList7 = list.subList( 1, num );
        Assert.assertEquals( actList.size(), 10, actList.toString() );
        for ( int i = 0; i < 10; i++ ) {
            Document act = actList.get( i );
            Document exp = expList7.get( expList7.size() - 2 - i );
            Assert.assertEquals( act.get( "a" ), exp.get( "a" ) );
            Assert.assertEquals( act.get( "b" ), exp.get( "b" ) );
            Assert.assertEquals( act.get( "c" ), exp.get( "c" ) );
            Assert.assertEquals( act.get( "_id" ), exp.get( "_id" ) );
            Assert.assertNull( act.get( "d" ) );
        }
    }

    @Test
    @SuppressWarnings("unchecked")
    public void test2() {
        FindIterable< Document > actFindIterable;
        List< Document > actList;
        // 存在的字段,匹配到记录，排除_id
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).projection( excludeId() )
                .skip( 1 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames1 = new String[] { "a", "b", "c", "d", "e" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames1 );

        // 存在的字段,匹配不到记录，排除_id
        actFindIterable = cl.find( gt( "a", num ) )
                .sort( Sorts.ascending( "a" ) ).projection( excludeId() )
                .skip( 1 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 0 );

        // 存在字段，排除其它字段
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( exclude( "a", "b" ) ) ).skip( 1 )
                .limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames3 = new String[] { "c", "d", "_id", "e" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames3 );

        // 存在字段，选择id字段
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( include( "_id" ) ) ).skip( 1 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames4 = new String[] { "_id" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames4 );

        // 存在字段，选择普通字段
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( include( "a", "b" ) ) ).skip( 1 )
                .limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames5 = new String[] { "a", "b", "_id" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames5 );

        // 排除_id和选择其他字段
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( include( "a", "b" ), excludeId() ) )
                .skip( 1 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames6 = new String[] { "a", "b" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames6 );

        // 选择_id和选择其他字段
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( include( "a", "b", "_id" ) ) ).skip( 1 )
                .limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames7 = new String[] { "a", "b", "_id" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames7 );

        // 选择_id和排除其他字段
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( exclude( "a", "b" ), include( "_id" ) ) )
                .skip( 1 ).limit( 10 );
        try {
            actFindIterable.into( new ArrayList< Document >() );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( MongoQueryException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }

        // 排除其它字段，选择其它字段
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( exclude( "a", "b" ), include( "c" ) ) )
                .skip( 1 ).limit( 10 );
        try {
            actFindIterable.into( new ArrayList< Document >() );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( MongoQueryException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }

        // 选择的字段不存在
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( include( "k" ) ) ).skip( 1 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames10 = new String[] { "_id" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames10 );

        // 排除的字段不存在
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( exclude( "k" ) ) ).skip( 1 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ) );

        // 选择字段部分字段存在，部分字段不存在
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( include( "k", "a" ) ) ).skip( 1 )
                .limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames12 = new String[] { "a", "_id" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames12 );

        // 排除字段部分字段存在，部分字段不存在
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( fields( exclude( "k", "a" ) ) ).skip( 1 )
                .limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        String[] includeNames13 = new String[] { "b", "c", "d", "_id", "e" };
        checkFindResult( actList, list.subList( 2, Math.min( 12, num ) ),
                includeNames13 );

        // 选择符：$elemMatch
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( new Document( "e", new Document( "$elemMatch",
                        new Document( "a", 1 ) ) ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), list.size(),
                "act = " + actList + ",exp = " + list );
        for ( int i = 0; i < actList.size(); i++ ) {
            Document act = actList.get( 0 );
            Document exp = list.get( 0 );
            Assert.assertEquals( ( ( Document ) act.get( "e" ) ).get( "a" ),
                    ( ( Document ) exp.get( "e" ) ).get( "a" ) );
            Assert.assertNull( ( ( Document ) act.get( "e" ) ).get( "b" ) );
            Assert.assertEquals( act.get( "a" ), exp.get( "a" ) );
            Assert.assertEquals( act.get( "b" ), exp.get( "b" ) );
            Assert.assertEquals( act.get( "c" ), exp.get( "c" ) );
            Assert.assertEquals( act.get( "d" ), exp.get( "d" ) );
        }

        // 选择符：$slice
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) )
                .projection( new Document( "c", new Document( "$slice", 1 ) ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), list.size(),
                "act = " + actList + ",exp = " + list );
        for ( int i = 0; i < actList.size(); i++ ) {
            Document act = actList.get( i );
            Document exp = list.get( i );
            Assert.assertEquals( act.get( "e" ), exp.get( "e" ) );
            Assert.assertEquals( act.get( "a" ), exp.get( "a" ) );
            Assert.assertEquals( act.get( "b" ), exp.get( "b" ) );
            Assert.assertEquals( act.get( "d" ), exp.get( "d" ) );
            List< Integer > cList = ( List< Integer > ) act.get( "c" );
            Assert.assertEquals( cList.size(), 1, cList.toString() );
            Assert.assertEquals( cList.get( 0 ),
                    ( ( List< Integer > ) exp.get( "c" ) ).get( 0 ) );
        }
    }

    @Test
    public void test3() {
        FindIterable< Document > actFindIterable;
        List< Document > actList;
        // 带匹配符+sort查询
        // 单个字段正序查询
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        checkFindResult( actList, list );

        // 单个字段逆序查询
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.descending( "a" ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        List< Document > expList = new ArrayList<>( list );
        Collections.reverse( expList );
        checkFindResult( actList, expList );

        // 多个字段正逆序
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.orderBy( Sorts.ascending( "a" ),
                        Sorts.descending( "b" ) ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        checkFindResult( actList, list );

        // 不存在的字段排序
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.descending( "a1" ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list );
    }

    @Test
    public void test4() {
        FindIterable< Document > actFindIterable;
        List< Document > actList;
        // 带匹配符、sort、limit、skip查询
        // skip大于等于匹配的记录数
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).skip( num ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 0 );

        // skip小于匹配数，limit小于匹配数
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).skip( num / 3 )
                .limit( num / 2 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList,
                list.subList( num / 3, num / 3 + num / 2 ) );

        // skip小于匹配数，limit大于匹配数
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).skip( num / 3 )
                .limit( num * 5 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( num / 3, num ) );

        // 匹配的记录数是0，skip limit
        actFindIterable = cl.find( and( gt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).skip( num / 3 )
                .limit( num * 5 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 0 );
        // limit为0
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .limit( 0 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 0, num ) );
        // limit为-1
        actFindIterable = cl.find( and( gte( "a", 0 ), lt( "a", num ) ) )
                .limit( -1 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 0, 1 ) );
    }

    @Test
    public void test6() {
        FindIterable< Document > actFindIterable;
        List< Document > actList;
        // 带不存在的字段进行查询
        actFindIterable = cl.find( and( gte( "a1", 0 ) ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 0 );

        // 集合不存在数据，进行查询
        String clName1 = clName + "_test6";
        db.createCollection( clName1 );
        MongoCollection< Document > cl1 = db.getCollection( clName1 );
        actFindIterable = cl.find( and( gte( "a1", 0 ) ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 0 );
        cl1.drop();
    }

    @Test
    public void test7() {
        FindIterable< Document > actFindIterable;
        List< Document > actList;
        // 带_id查询
        actFindIterable = cl.find( eq( "_id", list.get( 0 ).get( "_id" ) ) );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 1, actList.toString() );
        Assert.assertEquals( actList.get( 0 ), list.get( 0 ) );
    }

    @Test
    public void test8() {
        FindIterable< Document > actFindIterable;
        List< Document > actList;
        // 带batchSize、limit查询
        // batchSize小于limit的场景
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).batchSize( 5 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 1, 11 ) );
        // batchSize等于limit的场景
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).batchSize( 10 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 1, 11 ) );
        // batchSize大于limit的场景
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).batchSize( 20 ).limit( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 1, 11 ) );
        // 存在的字段，匹配不到记录，batchSize不为0
        actFindIterable = cl.find( gt( "a", num ) )
                .sort( Sorts.ascending( "a" ) ).batchSize( 10 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList.size(), 0 );
        // batchSize为0的场景
        actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
                .sort( Sorts.ascending( "a" ) ).batchSize( 0 );
        actList = actFindIterable.into( new ArrayList< Document >() );
        Assert.assertEquals( actList, list.subList( 1, num ) );

        // batchSize为-1的场景
        // TODO: SEQUOIADBMAINSTREAM-5977
        // actFindIterable = cl.find( and( gte( "a", 1 ), lt( "a", num ) ) )
        // .sort( Sorts.ascending( "a" ) )
        // .batchSize( -1 );
        // actList = actFindIterable.into( new ArrayList< Document >() );
        // Assert.assertEquals( actList, list.get( 0 ) );
    }

    private void checkFindResult( List< Document > actList,
            List< Document > expList, String... includeFieldNames ) {
        if ( expList == null || expList.isEmpty() ) {
            Assert.fail( "expList is null or empty" );
        }
        Set< String > actKeySet = actList.get( 0 ).keySet();
        Set< String > keySet = expList.get( 0 ).keySet();
        Assert.assertEquals( actList.size(), expList.size(), "actList = "
                + actList.toString() + ",expList = " + expList.toString() );
        Assert.assertTrue( actKeySet.size() <= keySet.size(),
                "actKeySet  " + actKeySet + ",keySet = " + keySet );
        if ( includeFieldNames != null && includeFieldNames.length == 0 ) {
            for ( int i = 0; i < expList.size(); i++ ) {
                Document act = actList.get( i );
                Document exp = expList.get( i );
                Assert.assertEquals( act, exp );
            }
        } else {
            List< String > excludeFieldNames = new ArrayList<>();
            excludeFieldNames.addAll( keySet );
            excludeFieldNames.removeAll( Arrays.asList( includeFieldNames ) );
            for ( int i = 0; i < expList.size(); i++ ) {
                Document act = actList.get( i );
                Document exp = expList.get( i );
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
