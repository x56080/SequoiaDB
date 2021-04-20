package com.mongodb.java;

import static com.mongodb.client.model.Filters.eq;
import static com.mongodb.client.model.Filters.gte;
import static com.mongodb.client.model.Updates.inc;
import static com.mongodb.client.model.Updates.set;

import java.math.BigDecimal;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Random;

import org.bson.BsonArray;
import org.bson.BsonBinary;
import org.bson.BsonBoolean;
import org.bson.BsonDateTime;
import org.bson.BsonDecimal128;
import org.bson.BsonDocument;
import org.bson.BsonDouble;
import org.bson.BsonInt32;
import org.bson.BsonMaxKey;
import org.bson.BsonMinKey;
import org.bson.BsonNull;
import org.bson.BsonObjectId;
import org.bson.BsonString;
import org.bson.BsonTimestamp;
import org.bson.Document;
import org.bson.conversions.Bson;
import org.bson.types.Decimal128;
import org.bson.types.ObjectId;
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
import com.mongodb.client.model.Sorts;
import com.mongodb.client.model.Updates;
import com.mongodb.client.result.UpdateResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-22419:数据类型测试
 * @author fanyu
 * @version 1.00
 * @Date 2020/7/3
 */
public class AllDataType22419 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName1;
    private String clName2;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName1 = javaDBNameWithVersion + "_cl22419A";
        clName2 = javaDBNameWithVersion + "_cl22419B";
    }

    @Test
    public void test1() {
        MongoCollection< Document > cl = db.getCollection( clName1 );
        BsonDocument bsonDocument = new BsonDocument()
                .append( "decimalFiled1",
                        new BsonDecimal128( new Decimal128(
                                new Long( 3476778912330022912L ) ) ) )
                .append( "decimalFiled2",
                        new BsonDecimal128( new Decimal128(
                                new Long( 8646911284551352320L ) ) ) )
                .append( "decimalFiled3",
                        new BsonDecimal128(
                                new Decimal128( new BigDecimal( 8646911 ) ) ) )
                .append( "decimalFiled5",
                        new BsonDecimal128( Decimal128.POSITIVE_INFINITY ) )
                .append( "decimalFiled6",
                        new BsonDecimal128( Decimal128.NEGATIVE_INFINITY ) )
                .append( "decimalFiled7",
                        new BsonDecimal128( Decimal128.NEGATIVE_NaN ) )
                .append( "decimalFiled8", new BsonDecimal128( Decimal128.NaN ) )
                .append( "decimalFiled8",
                        new BsonDecimal128( Decimal128.parse( "10" ) ) )
                .append( "decimalFiled9",
                        new BsonDecimal128( Decimal128.POSITIVE_ZERO ) )
                // TODO:SEQUOIADBMAINSTREAM-6037
                // .append( "decimalFiled10",
                // new BsonDecimal128( Decimal128.NEGATIVE_ZERO ) )
                .append( "decimalFiled11",
                        new BsonDecimal128(
                                Decimal128.parse( "0.471447736024856578" ) ) )
                .append( "decimalFiled12",
                        new BsonDecimal128( Decimal128.parse( "1E-999" ) ) )
                .append( "decimalFiled13",
                        new BsonDecimal128( Decimal128.parse( "0" ) ) )
                .append( "decimalFiled14",
                        new BsonDecimal128(
                                Decimal128.parse( "-1.471447736024856578" ) ) )
                .append( "decimalFiled15",
                        new BsonDecimal128(
                                Decimal128.parse( "1.5677788E+999" ) ) )
                .append( "decimalFiled16",
                        new BsonDecimal128(
                                Decimal128.parse( "1.5677788E+1000" ) ) )
                .append( "decimalFiled17",
                        new BsonDecimal128(
                                Decimal128.parse( "1.5677788E-992" ) ) )
                .append( "decimalFiled18",
                        new BsonDecimal128(
                                Decimal128.parse( "1.5677788E-993" ) ) )
                .append( "decimalFiled19",
                        new BsonDecimal128(
                                Decimal128.parse( "-2.5677788E+999" ) ) )
                .append( "decimalFiled20",
                        new BsonDecimal128(
                                Decimal128.parse( "-2.5677788E+1000" ) ) )
                .append( "decimalFiled21",
                        new BsonDecimal128(
                                Decimal128.parse( "-2.5677788E-992" ) ) )
                .append( "decimalFiled22", new BsonDecimal128(
                        Decimal128.parse( "-2.5677788E-993" ) ) );

        Document document = Document.parse( bsonDocument.toJson() );
        cl.insertOne( document );

        // 查询
        List< Document > list = cl.find().into( new ArrayList< Document >() );
        // 检查结果
        // TODO:SEQUOIADBMAINSTREAM-6037
        document.append( "decimalFiled15", Decimal128
                .parse( "1.567778800000000000000000000000000E+999" ) );
        document.append( "decimalFiled16", Decimal128
                .parse( "1.567778800000000000000000000000000E+1000" ) );
        document.append( "decimalFiled19", Decimal128
                .parse( "-2.567778800000000000000000000000000E+999" ) );
        document.append( "decimalFiled20", Decimal128
                .parse( "-2.567778800000000000000000000000000E+1000" ) );
        Assert.assertEquals( list.size(), 1, list.toString() );
        Assert.assertEquals( list.get( 0 ), document );

        // 更新
        Bson query = Filters.and(
                gte( "decimalFiled5", Decimal128.POSITIVE_INFINITY ),
                // TODO:SEQUOIADBMAINSTREAM-6037
                // gte( "decimalFiled7", Decimal128.NEGATIVE_NaN ),
                gte( "decimalFiled9", Decimal128.POSITIVE_ZERO ),
                gte( "decimalFiled11",
                        Decimal128.parse( "0.471447736024856578" ) ),
                gte( "decimalFiled16",
                        Decimal128.parse(
                                "1.567778800000000000000000000000000E+1000" ) ),
                gte( "decimalFiled18", Decimal128.parse( "1.5677788E-993" ) ) );

        Bson update = Updates.combine( inc( "decimalFiled11", 1 ),
                set( "decimalFiled16", Decimal128.parse(
                        "2.567778800000000000000000000000000E+1000" ) ) );

        UpdateResult updateResult = cl.updateMany( query, update );
        Assert.assertEquals( updateResult.getMatchedCount(), 1 );
        Assert.assertEquals( updateResult.getModifiedCount(), 1 );

        // 检查结果
        List< Document > list1 = cl.find().into( new ArrayList< Document >() );
        document.append( "decimalFiled11",
                Decimal128.parse( "1.471447736024856578" ) );
        document.append( "decimalFiled16", Decimal128
                .parse( "2.567778800000000000000000000000000E+1000" ) );
        Assert.assertEquals( list1.size(), 1, list.toString() );
        Assert.assertEquals( list1.get( 0 ), document );

        // 聚集操作
        Document document1 = Document.parse( bsonDocument.toJson() );
        cl.insertOne( document1 );
        List< Document > actResult = cl
                .aggregate( Arrays.asList(
                        Aggregates.match( eq( "decimalFiled5",
                                Decimal128.POSITIVE_INFINITY ) ),
                        Aggregates.group( "$decimalFiled5",
                                Accumulators.sum( "sum_decimalFiled11",
                                        "$decimalFiled11" ) ),
                        Aggregates.sort( Sorts.ascending( "_id" ) ) ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actResult.size(), 1, actResult.toString() );
        Assert.assertEquals( actResult.get( 0 ),
                new Document().append( "_id", Decimal128.POSITIVE_INFINITY )
                        .append( "sum_decimalFiled11",
                                Decimal128.parse( "1.942895472049713156" ) ) );

        // 删除操作
        cl.deleteMany( query );
        // 检查结果
        Assert.assertEquals( cl.count( query ), 0 );

        try {
            cl.insertOne( new Document( "decimalFiled1001",
                    Decimal128.parse( "1.5677788E-1001" ) ) );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( MongoCommandException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }
    }

    @Test
    public void test2() {
        MongoCollection< Document > cl = db.getCollection( clName2 );
        byte[] bytes = new byte[ 5 ];
        new Random().nextBytes( bytes );
        BsonDocument bsonDocument = new BsonDocument()
                .append( "stringFiled", new BsonString( "text" ) )
                .append( "arrayFiled",
                        new BsonArray(
                                Arrays.asList( new BsonInt32( -2147483648 ),
                                        new BsonInt32( 2147483647 ) ) ) )
                .append( "dateFiled", new BsonDateTime( new Date().getTime() ) )
                .append( "booleanFiled", new BsonBoolean( false ) )
                .append( "doubleFiled", new BsonDouble( 0.47144 ) )
                .append( "binaryFiled", new BsonBinary( bytes ) )
                .append( "ObjectIdFiled",
                        new BsonObjectId(
                                new ObjectId( "5ebe033bc5199b5af1e35876" ) ) )
                .append( "NullFiled", new BsonNull() )
                .append( "timestampFiled", new BsonTimestamp( 10, 8 ) )
                .append( "maxKeyFiled", new BsonMaxKey() )
                .append( "minKeyFiled", new BsonMinKey() );
        Document document = Document.parse( bsonDocument.toJson() );

        // 插入
        cl.insertOne( document );

        // 查询
        List< Document > list = cl.find().into( new ArrayList< Document >() );
        Assert.assertEquals( list.size(), 1, list.toString() );
        Assert.assertEquals( list.get( 0 ), document );

        // 更新
        Bson update = Updates.combine(
                set( "arrayFiled",
                        new BsonArray( Arrays.asList( new BsonInt32( 123 ),
                                new BsonInt32( 456 ) ) ) ),
                set( "booleanFiled", true ),
                set( "timestampFiled", new BsonTimestamp( 1000, 8 ) ) );
        cl.updateMany( document, update );

        // 检查结果
        document.append( "booleanFiled", true );
        document.append( "timestampFiled", new BsonTimestamp( 1000, 8 ) );
        document.append( "arrayFiled", Arrays.asList( 123, 456 ) );
        list = cl.find().into( new ArrayList< Document >() );
        Assert.assertEquals( list.size(), 1, list.toString() );
        Assert.assertEquals( list.get( 0 ), document );

        // 删除操作
        cl.deleteMany( document );
        // 检查结果
        Assert.assertEquals( cl.count(), 0 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName1, clName2 );
    }
}
