package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

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
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21930:aggregate操作
 * @author fanyu
 * @Date 2020/3/17
 * @version 1.00
 */

public class Aggregate21930 extends MongodbTestBase {
    private DB db;
    private String clName;
    private DBCollection cl;
    // 不能小于6
    private int num = 6;
    private List< DBObject > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        clName = javaDBNameWithVersion + "_cl21930";

        list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", "" + i % 3 )
                    .append( "c", new int[] { i + 1, i + 2, i + 3 } )
                    .append( "d", i ) );
        }
        cl = db.getCollection( clName );
        cl.insert( list );
        cl.createIndex( new BasicDBObject( "a", 1 ), "a" );
        cl.createIndex( new BasicDBObject( "b", 1 ).append( "f", 1 ), "bf" );
    }

    @Test
    public void test1() {
        List< DBObject > aggList;
        Iterator< DBObject > actResult;
        int k;
        // 带匹配符
        // 匹配到记录
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", QueryBuilder.start( "a" )
                .greaterThanEquals( 0 ).lessThan( num ).get() ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            DBObject act = actResult.next();
            DBObject exp = list.get( k++ );
            Assert.assertEquals( act, exp );
        }
        Assert.assertEquals( k, num );

        // 匹配不到记录
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).lessThan( -1 ).get() ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        Assert.assertFalse( actResult.hasNext() );

        // 带匹配符+选择符进行聚集
        // （1）选择普通字段
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add( new BasicDBObject( "$project",
                new BasicBSONObject( "a", 1 ).append( "d", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            DBObject act = actResult.next();
            DBObject exp = list.get( k );
            Assert.assertEquals( act,
                    new BasicDBObject( "a", exp.get( "a" ) )
                            .append( "d", exp.get( "d" ) )
                            .append( "_id", exp.get( "_id" ) ) );
            k++;
        }
        Assert.assertEquals( k, num );

        // （2）选择_id字段
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add( new BasicDBObject( "$project",
                new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            DBObject act = actResult.next();
            DBObject exp = list.get( k );
            Assert.assertEquals( act,
                    new BasicDBObject( "_id", exp.get( "_id" ) ) );
            k++;
        }
        Assert.assertEquals( k, num );

        // （3）排除普通字段
        try {
            aggList = new ArrayList<>();
            aggList.add( new BasicDBObject( "$match",
                    QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
            aggList.add( new BasicDBObject( "$project",
                    new BasicBSONObject( "a", 0 ) ) );
            cl.aggregate( aggList ).results().iterator();
            Assert.fail( "expect failed but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-32" ) ) {
                throw e;
            }
        }
        // （4）排除_id字段
        try {
            aggList = new ArrayList<>();
            aggList.add( new BasicDBObject( "$match",
                    QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
            aggList.add( new BasicDBObject( "$project",
                    new BasicBSONObject( "_id", 0 ) ) );
            cl.aggregate( aggList ).results().iterator();
            Assert.fail( "expect failed but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-32" ) ) {
                throw e;
            }
        }

        // （5）选择普通字段和排除_id
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add( new BasicDBObject( "$project",
                new BasicBSONObject( "_id", 0 ).append( "a", 1 )
                        .append( "b", 1 ).append( "c", 1 ).append( "d", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            DBObject act = actResult.next();
            DBObject exp = list.get( k );
            Assert.assertEquals( act, new BasicBSONObject( "a", exp.get( "a" ) )
                    .append( "b", exp.get( "b" ) ).append( "c", exp.get( "c" ) )
                    .append( "d", exp.get( "d" ) ) );
            k++;
        }
        Assert.assertEquals( k, num );

        // （6）排除普通字段和排除_id
        try {
            aggList = new ArrayList<>();
            aggList.add( new BasicDBObject( "$match",
                    QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
            aggList.add( new BasicDBObject( "$project",
                    new BasicBSONObject( "_id", 0 ).append( "a", 0 ) ) );
            cl.aggregate( aggList ).results().iterator();
            Assert.fail( "exp failed but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-32" ) ) {
                throw e;
            }
        }

        // （7）选择_id字段和选择普通字段
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add(
                new BasicDBObject( "$project", new BasicBSONObject( "_id", 1 )
                        .append( "a", 1 ).append( "b", 1 ).append( "c", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            DBObject act = actResult.next();
            DBObject exp = list.get( k );
            Assert.assertEquals( act, new BasicBSONObject( "a", exp.get( "a" ) )
                    .append( "b", exp.get( "b" ) ).append( "c", exp.get( "c" ) )
                    .append( "_id", exp.get( "_id" ) ) );
            k++;
        }
        Assert.assertEquals( k, num );

        // 带匹配符+选择符+sort+skip+limit + 选择_id字段
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add( new BasicDBObject( "$project",
                new BasicBSONObject( "_id", 1 ).append( "a", 1 )
                        .append( "b", 1 ).append( "c", 1 ).append( "d", 1 ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicDBObject( "a", -1 ) ) );
        aggList.add( new BasicDBObject( "$skip", 1 ) );
        aggList.add( new BasicDBObject( "$limit", num ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = num - 2;
        while ( actResult.hasNext() ) {
            Assert.assertEquals( actResult.next(), list.get( k ) );
            k--;
        }
        Assert.assertEquals( k, -1 );

        // 带分组进行聚集
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicDBObject( "_id", "$b" ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicDBObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ) );
            k++;
        }
        Assert.assertEquals( k, 3 );
    }

    @Test
    public void test2() {
        List< DBObject > aggList;
        Iterator< DBObject > actResult;
        int k;
        // 带匹配符、分组、选择符、聚集符进行聚集
        // lte group avg sort
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "avg_d",
                        new BasicBSONObject( "$avg", "$d" ) ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        double sum8 = 0;
        int j8 = 0;
        while ( actResult.hasNext() ) {
            for ( int i = 0; i < num; i++ ) {
                if ( i % 3 == k ) {
                    sum8 += i;
                    j8++;
                }
            }
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "avg_d",
                            sum8 / j8 ) );
            k++;
            sum8 = 0;
            j8 = 0;
        }
        Assert.assertEquals( k, 3 );

        // lte group avg sum sort
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "sum_d",
                        new BasicBSONObject( "$sum", "$d" ) ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        double sum9 = 0;
        while ( actResult.hasNext() ) {
            for ( int i = 0; i < num; i++ ) {
                if ( i % 3 == k ) {
                    sum9 += i;
                }
            }
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "sum_d",
                            sum9 ) );
            k++;
            sum9 = 0;
        }
        Assert.assertEquals( k, 3 );

        // lte group avg max
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "max_d",
                        new BasicBSONObject( "$max", "$d" ) ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        int max10 = 0;
        while ( actResult.hasNext() ) {
            for ( int i = 0; i < num; i++ ) {
                if ( i % 3 == k ) {
                    max10 = i;
                }
            }
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "max_d",
                            max10 ) );

            k++;
        }
        Assert.assertEquals( k, 3 );

        // lte group avg min
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "min_d",
                        new BasicBSONObject( "$min", "$d" ) ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "min_d",
                            k % 3 ) );
            k++;
        }
        Assert.assertEquals( k, 3 );

        // lte group addtoset
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "set",
                        new BasicBSONObject( "$addToSet", "$d" ) ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        List< Integer > expList12 = new ArrayList<>();
        while ( actResult.hasNext() ) {
            for ( int i = num - 1; i >= 0; i-- ) {
                if ( i % 3 == k ) {
                    expList12.add( i );
                }
            }
            Collections.reverse( expList12 );
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "set",
                            expList12 ) );
            expList12.clear();
            k++;
        }
        Assert.assertEquals( k, 3 );

        // lte group first
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$sort",
                new BasicBSONObject( "first_d", 1 ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "first_d",
                        new BasicBSONObject( "$first", "$d" ) ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "first_d",
                            k % 3 ) );
            k++;
        }
        Assert.assertEquals( k, 3, "$first agg = " + aggList.toString() );

        // lte group last
        // TODO:SEQUOIADBMAINSTREAM-5656
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$sort",
                new BasicBSONObject( "last_d", 1 ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "last_d",
                        new BasicBSONObject( "$last", "$d" ) ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        // int k14 = 0;
        // int last14 = 0;
        // while ( result14.hasNext() ){
        // for(int i = 0; i < num; i++){
        // if(i%3 == k14){
        // last14 = i;
        // }
        // }
        // Assert.assertEquals( result14.next(), new BasicDBObject( "_id",
        // k14+"" ).append( "last_d",last14 ));
        //
        // k14++;
        // }
        // Assert.assertEquals( k14,3 );

        // lte group push
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "push",
                        new BasicBSONObject( "$push", "$d" ) ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        List< Integer > expList15 = new ArrayList<>();
        while ( actResult.hasNext() ) {
            for ( int i = 0; i < num; i++ ) {
                if ( i % 3 == k ) {
                    expList15.add( i );
                }
            }
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "push",
                            expList15 ) );
            expList15.clear();
            k++;
        }
        Assert.assertEquals( k, 3, "$last agg = " + aggList.toString() );
    }

    @Test
    public void test3() {
        List< DBObject > aggList;
        Iterator< DBObject > actResult;
        int k;
        // 集合不存在数据，进行聚集
        if ( db.collectionExists( clName + "-test3" ) ) {
            db.getCollection( clName + "-test3" ).drop();
        }
        DBCollection cl1 = db.createCollection( clName + "-test3",
                new BasicDBObject() );
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", QueryBuilder.start( "a" )
                .greaterThanEquals( 0 ).lessThan( num ).get() ) );
        actResult = cl1.aggregate( aggList ).results().iterator();
        Assert.assertFalse( actResult.hasNext() );
        cl1.drop();

        // 匹配字段不存在，进行聚集
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", QueryBuilder.start( "a1" )
                .greaterThanEquals( 0 ).lessThan( num ).get() ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            Assert.assertEquals( actResult.next(), list.get( k ) );
            k++;
        }

        // 分组的字段不存在，进行聚集
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicDBObject( "_id", "$l" ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        Assert.assertTrue( actResult.hasNext() );
        Assert.assertEquals( actResult.next().keySet().size(), 1 );

        // 选择字段不存在，进行聚集
        // 选择的字段不存在 会返回_id字段
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match",
                QueryBuilder.start( "a" ).greaterThan( -1 ).get() ) );
        aggList.add( new BasicDBObject( "$project",
                new BasicBSONObject( "_a", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        Set< String > expSet = new HashSet<>();
        expSet.add( "_id" );
        expSet.add( "_a" );
        while ( actResult.hasNext() ) {
            Set< String > keySet = actResult.next().keySet();
            Assert.assertEquals( keySet, expSet, "act = " + keySet );
            k++;
        }
        Assert.assertEquals( k, num );

        // 聚集字段不存在
        aggList = new ArrayList<>();
        aggList.add( new BasicDBObject( "$match", new BasicBSONObject( "a",
                new BasicBSONObject( "$lte", num ) ) ) );
        aggList.add( new BasicDBObject( "$group",
                new BasicBSONObject( "_id", "$b" ).append( "max_d",
                        new BasicBSONObject( "$max", "$d1" ) ) ) );
        aggList.add(
                new BasicDBObject( "$sort", new BasicBSONObject( "_id", 1 ) ) );
        actResult = cl.aggregate( aggList ).results().iterator();
        k = 0;
        while ( actResult.hasNext() ) {
            Assert.assertEquals( actResult.next(),
                    new BasicDBObject( "_id", k + "" ).append( "max_d",
                            null ) );
            k++;
        }
        Assert.assertEquals( k, 3 );
    }

    @Test
    private void test4() {
        // 参数校验
        List< DBObject > list = new ArrayList<>();
        list.add( new BasicDBObject( "$match",
                QueryBuilder.start( "$" ).greaterThan( -1 ).get() ) );
        list.add( new BasicDBObject( "$project",
                new BasicBSONObject( "_a", 1 ) ) );
        try {
            cl.aggregate( list ).results().iterator();
            Assert.fail( "exp failed but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-6" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
