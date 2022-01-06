package com.mongodb.java;

import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Filters.lt;
import static com.mongodb.client.model.Projections.exclude;
import static com.mongodb.client.model.Projections.excludeId;
import static com.mongodb.client.model.Projections.fields;
import static com.mongodb.client.model.Projections.include;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Set;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.MongoCommandException;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Accumulators;
import com.mongodb.client.model.Aggregates;
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.client.model.Sorts;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21930:aggregate操作
 * @author fanyu
 * @version 1.00
 * @Date 2020/3/24
 */
public class Aggregate21930 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    // 不能小于6
    private int num = 50;
    private List< Document > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl21930";
        list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new Document( "a", i ).append( "b", "" + i % 3 )
                    .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
                    .append( "d", i ) );
        }
        cl = db.getCollection( clName );
        cl.insertMany( list );
        cl.createIndex( Indexes.descending( "a" ),
                new IndexOptions().unique( false ).name( "a" ) );
    }

    @Test
    public void test1() {
        List< Document > actResult;
        // 带匹配符
        // 匹配到记录
        Collection< Document > result = cl
                .aggregate( Arrays.asList(
                        Aggregates.match(
                                Filters.and( lt( "a", num ), gte( "a", 0 ) ) ),
                        Aggregates.sort( Sorts.ascending( "a" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( result, list );

        // 匹配不到记录
        result = cl
                .aggregate( Arrays.asList( Aggregates.match( lt( "a", -1 ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( result.size(), 0 );

        // 带匹配符+选择符进行聚集
        // （1）选择普通字段
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.project( include( "a", "b" ) ),
                        Aggregates.sort( Sorts.ascending( "a" ) ) ) )
                .into( new ArrayList< Document >() );
        checkSelectResults( actResult, list, new String[] { "_id", "a", "b" } );

        // （2）选择_id字段
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.project( include( "_id" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        checkSelectResults( actResult, list, new String[] { "_id" } );

        // （4）排除_id字段
        try {
            actResult = ( List< Document > ) cl
                    .aggregate(
                            Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                                    Aggregates.project( excludeId() ),
                                    Aggregates
                                            .sort( Sorts.ascending( "a" ) ) ) )
                    .into( new ArrayList< Document >() );
            Assert.fail( "expect failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -32 ) {
                throw e;
            }
        }
        // sequoiadb和mongodb返回的结果不一致
        // Sequoiadb: [Document{{_id=5e9420a8cc4da963bc509c1c, a=0, b=0, c=[1,2,
        // 3], d=0}}
        // Mongodb：[Document{{a=0, b=0, c=[1, 2, 3], d=0}},
        // 跟开发确认不改

        // （3）排除其它普通字段
        try {
            actResult = ( List< Document > ) cl
                    .aggregate( Arrays.asList(
                            Aggregates.match( gte( "a", num / 2 ) ),
                            Aggregates.project( exclude( "a", "b" ) ),
                            Aggregates.sort( Sorts.ascending( "d" ) ) ) )
                    .into( new ArrayList< Document >() );
            Assert.fail( "expect failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -32 ) {
                throw e;
            }
        }

        // （5）选择普通字段和排除_id
        actResult = ( List< Document > ) cl
                .aggregate(
                        Arrays.asList( Aggregates.match( gte( "a", num / 2 ) ),
                                Aggregates.project(
                                        fields( excludeId(), include( "a" ) ) ),
                                Aggregates.sort( Sorts.ascending( "a" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), num / 2 );
        checkSelectResults( actResult, list.subList( num / 2, num ),
                new String[] { "a" } );

        // // （6）排除普通字段和排除_id
        try {
            actResult = ( List< Document > ) cl
                    .aggregate( Arrays.asList(
                            Aggregates.match( gte( "a", num / 2 ) ),
                            Aggregates.project( fields( exclude( "a", "b" ),
                                    excludeId() ) ),
                            Aggregates.sort( Sorts.ascending( "a" ) ) ) )
                    .into( new ArrayList< Document >() );
            Assert.fail( "expect failed but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -32 ) {
                throw e;
            }
        }
        // （7）选择_id字段和选择普通字段
        actResult = ( List< Document > ) cl
                .aggregate(
                        Arrays.asList( Aggregates.match( gte( "a", num / 2 ) ),
                                Aggregates.project(
                                        fields( include( "_id", "a", "b" ) ) ),
                                Aggregates.sort( Sorts.ascending( "a" ) ) ) )
                .into( new ArrayList< Document >() );
        checkSelectResults( actResult, list.subList( num / 2, num ),
                new String[] { "_id", "a", "b" } );

        // 带匹配符+选择符+sort+skip+limit
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList(
                        Aggregates.match( gte( "a", num / 2 ) ),
                        Aggregates.project(
                                fields( excludeId(), include( "a" ) ) ),
                        Aggregates.sort( Sorts.ascending( "a" ) ),
                        Aggregates.skip( 1 ), Aggregates.limit( num / 2 ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), num / 2 - 1 );
        checkSelectResults( actResult, list.subList( num / 2 + 1, num ), "a" );

        // 带分组进行聚集
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b" ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ),
                        Aggregates.skip( 1 ), Aggregates.limit( num / 2 ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 2 );
        for ( int i = 0; i < 2; i++ ) {
            Assert.assertEquals( actResult.get( i ).toString(),
                    new Document( "_id", i + 1 ).toString() );
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void test2() {
        List< Document > actResult;
        // 带匹配符、分组、选择符、聚集符进行聚集
        // gte group avg sort
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.avg( "avg_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
        double sum1 = 0;
        double count1 = 0;
        for ( int i = 0; i < actResult.size(); i++ ) {
            for ( int j = 0; j < num; j++ ) {
                if ( j % 3 == i ) {
                    sum1 += j;
                    count1++;
                }
            }
            Assert.assertEquals( actResult.get( i ),
                    new Document( "_id", i + "" ).append( "avg_d",
                            sum1 / count1 ) );
            count1 = 0;
            sum1 = 0;
        }

        // gte group sum sort
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.sum( "sum_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
        double sum2 = 0;
        for ( int i = 0; i < actResult.size(); i++ ) {
            for ( int j = 0; j < num; j++ ) {
                if ( j % 3 == i ) {
                    sum2 += j;
                }
            }
            // mongodb返回的sum 是整型， sequoiadb返回的是浮点型
            Assert.assertEquals( actResult.get( i ),
                    new Document( "_id", i + "" ).append( "sum_d", sum2 ) );
            sum2 = 0;
        }

        // gte group max
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.max( "max_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
        int max3 = 0;
        for ( int i = 0; i < actResult.size(); i++ ) {
            for ( int j = 0; j < num; j++ ) {
                if ( j % 3 == i ) {
                    max3 = j;
                }
            }
            Assert.assertEquals( actResult.get( i ),
                    new Document( "_id", i + "" ).append( "max_d", max3 ) );
            max3 = 0;
        }

        // gte group avg min
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.min( "min_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
        int min4 = 0;
        for ( int i = 0; i < actResult.size(); i++ ) {
            for ( int j = 0; j < num; j++ ) {
                if ( j % 3 == i ) {
                    min4 = j;
                    break;
                }
            }
            Assert.assertEquals( actResult.get( i ),
                    new Document( "_id", i + "" ).append( "min_d", min4 ) );
            min4 = 0;
        }

        // gte group addtoset
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.addToSet( "set_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
        List< Integer > expList5 = new ArrayList<>();
        for ( int i = 0; i < actResult.size(); i++ ) {
            for ( int j = 0; j < num; j++ ) {
                if ( j % 3 == i ) {
                    expList5.add( j );
                }
            }
            Assert.assertEquals( actResult.get( i ).get( "_id" ), i + "",
                    actResult.toString() );
            List< Integer > actSet = ( ArrayList< Integer > ) actResult.get( i )
                    .get( "set_d" );
            Collections.sort( actSet );
            Assert.assertEquals( actSet, expList5, actResult.toString() );
            expList5.clear();
        }

        // gte group first
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.first( "first_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
        for ( int i = 0; i < actResult.size(); i++ ) {
            Assert.assertEquals( actResult.get( i ).get( "_id" ), i + "",
                    actResult.toString() );
            Assert.assertEquals(
                    0 <= actResult.get( i ).getInteger( "first_d" )
                            && actResult.get( i ).getInteger( "first_d" ) < num,
                    true, actResult.toString() );
        }

        // gte group last TODO:SEQUOIADBMAINSTREAM-5656
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.last( "last_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "last_d" ) ) ) )
                .into( new ArrayList< Document >() );
        // Assert.assertEquals( result7.size(), 3, "result7 = " +
        // actResult.toString() );
        // int last7 = 0;
        // for ( int i = 0; i < result7.size(); i++ ) {
        // for ( int j = 0; j < num; j++ ) {
        // if ( j % 3 == i ) {
        // last7 = j;
        // }
        // }
        // Assert.assertEquals( result7.get( i ), new Document( "_id", i + "" )
        // .append( "last_d", last7 ) );
        // last7 = 0;
        // }

        // gte group push
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.push( "push_d", "$d" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
        List< Integer > expList8 = new ArrayList<>();
        for ( int i = 0; i < actResult.size(); i++ ) {
            for ( int j = 0; j < num; j++ ) {
                if ( j % 3 == i ) {
                    expList8.add( j );
                }
            }
            // mongodb的expList8是逆序排序， sequoiadb的expList8是正序排序
            // Collections.reverse( expList8 );
            Assert.assertEquals( actResult.get( i ).get( "_id" ), i + "",
                    actResult.toString() );
            List< Integer > actPush = ( ArrayList< Integer > ) actResult
                    .get( i ).get( "push_d" );
            Collections.sort( actPush );
            Assert.assertEquals( actPush, expList8, actResult.toString() );
            expList8.clear();
        }
    }

    @Test
    public void test3() {
        List< Document > actResult;
        // 集合不存在数据，进行聚集
        String clName1 = clName + "-test3";
        List< String > clNames = db.listCollectionNames()
                .into( new ArrayList< String >() );
        if ( clNames.contains( clName1 ) ) {
            db.getCollection( clName + "-test3" ).drop();
        }
        db.createCollection( clName1 );
        MongoCollection< Document > cl1 = db.getCollection( clName1 );
        actResult = ( List< Document > ) cl1
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 0 );
        cl1.drop();

        // 匹配字段不存在，进行聚集
        actResult = ( List< Document > ) cl
                .aggregate( Collections
                        .singletonList( Aggregates.match( gte( "a1", -1 ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 0 );

        // 分组的字段不存在，进行聚集
        actResult = ( List< Document > ) cl
                .aggregate( Collections.singletonList( Aggregates.group( "$b1",
                        Accumulators.push( "push_d", "$d" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 1 );
        List< Integer > expList3 = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            expList3.add( i );
        }
        Assert.assertEquals( actResult.get( 0 ),
                new Document( "_id", null ).append( "push_d", expList3 ) );

        // 选择字段不存在，进行聚集
        // 选择的字段不存在 会返回_id字段
        actResult = ( List< Document > ) cl
                .aggregate( Collections.singletonList(
                        Aggregates.project( include( "a1" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), num );

        // 聚集字段不存在
        actResult = ( List< Document > ) cl
                .aggregate( Arrays.asList( Aggregates.match( gte( "a", -1 ) ),
                        Aggregates.group( "$b",
                                Accumulators.push( "push_d", "$d1" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 3 );
    }

    @Test
    private void test4() {
        // 参数校验
        try {
            cl.aggregate( Arrays.asList( Aggregates.match( gte( "$", -1 ) ),
                    Aggregates.group( "$b",
                            Accumulators.push( "push_d", "$d1" ) ),
                    Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                    .into( new ArrayList< Document >() );
            Assert.fail( "exp fail but act success" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }
    }

    private void checkSelectResults( List< Document > actList,
            List< Document > expList, String... includeFieldNames ) {
        if ( expList == null || expList.isEmpty() ) {
            Assert.fail( "expList is null or empty" );
        }
        Set< String > keySet = expList.get( 0 ).keySet();
        List< String > excludeFieldNames = new ArrayList<>();
        excludeFieldNames.addAll( keySet );
        excludeFieldNames.removeAll( Arrays.asList( includeFieldNames ) );
        Assert.assertEquals( actList.size(), expList.size(),
                "act = " + actList + ",exp = " + expList );
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

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
