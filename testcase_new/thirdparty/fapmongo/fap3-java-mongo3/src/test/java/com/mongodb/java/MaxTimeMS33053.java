package com.mongodb.java;

import static com.mongodb.client.model.Filters.gte;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.MongoCommandException;
import com.mongodb.MongoQueryException;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.InsertOneModel;
import com.mongodb.client.model.Sorts;
import com.mongodb.client.model.WriteModel;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33053:find/createIndexes操作时使用maxTimeMS参数
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class MaxTimeMS33053 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    private int docsNum = 50000;

    @BeforeClass
    private void setUp() {
        clName = javaDBNameWithVersion + "_cl_33053";
        db = MongodbTestBase.getDataBase( client );
        cl = db.getCollection( clName );
        cl.drop();
        db.createCollection( clName );

        List< WriteModel< Document > > writeMode = new ArrayList<>();
        for ( int i = 0; i < docsNum; i++ ) {
            Document insertDoc = new Document( "_id", i ).append( "a", i )
                    .append( "c", i );
            InsertOneModel< Document > insertOneMode = new InsertOneModel< Document >(
                    insertDoc );
            writeMode.add( insertOneMode );
        }
        cl.bulkWrite( writeMode );
    }

    @Test
    private void test() {
        this.testFind();
        this.testCreateIndexes();
        this.testDistinct();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }

    private void testFind() {
        long maxTimeMS;

        // maxTimeMS 大于实际执行时间
        maxTimeMS = 10 * 1000;
        int actDocsNum = cl.find( gte( "a", 100 ) )
                .sort( Sorts.descending( "a" ) )
                .maxTime( maxTimeMS, TimeUnit.MILLISECONDS )
                .into( new ArrayList< Document >() ).size();
        Assert.assertEquals( actDocsNum, docsNum - 100 );

        // maxTimeMS < 实际执行时间
        maxTimeMS = 10;
        try {
            cl.find( gte( "a", 100 ) ).sort( Sorts.descending( "a" ) )
                    .maxTime( maxTimeMS, TimeUnit.MILLISECONDS )
                    .into( new ArrayList< Document >() ).size();
        } catch ( MongoQueryException e ) {
            if ( -116 != e.getErrorCode() )
                throw e;
        }
    }

    private void testCreateIndexes() {
        long maxTimeMS;
        // 索引定义
        List< String > indexNames = Arrays.asList( "idx1", "idx2" );
        List< Document > indexes = new ArrayList<>();
        indexes.add( new Document( "key", new Document( "a", -1 ) )
                .append( "name", indexNames.get( 0 ) ) );
        indexes.add( new Document( "key", new Document( "b", -1 ) )
                .append( "name", indexNames.get( 1 ) ) );

        // maxTimeMS 大于实际执行时间
        maxTimeMS = 10 * 1000;
        db.runCommand( new Document( "createIndexes", clName )
                .append( "indexes", indexes )
                .append( "maxTimeMS", maxTimeMS ) );
        int indexNum = cl.listIndexes().into( new ArrayList< Document >() )
                .size();
        Assert.assertEquals( indexNum, 3 );
        for ( String name : indexNames )
            cl.dropIndex( name );

        // maxTimeMS < 实际执行时间
        maxTimeMS = 10;
        try {
            db.runCommand( new Document( "createIndexes", clName )
                    .append( "indexes", indexes )
                    .append( "maxTimeMS", maxTimeMS ) );
        } catch ( MongoCommandException e ) {
            if ( -116 != e.getErrorCode() )
                throw e;
        }
    }

    private void testDistinct() {
        long maxTimeMS;

        // maxTimeMS 大于实际执行时间
        maxTimeMS = 10 * 1000;
        Document doc = db.runCommand( new Document( "distinct", clName )
                .append( "key", "a" ).append( "maxTimeMS", maxTimeMS ) );
        @SuppressWarnings("unchecked")
        List< Integer > rcDocsNum = ( ArrayList< Integer > ) doc
                .get( "values" );
        Assert.assertEquals( rcDocsNum.size(), docsNum );

        // maxTimeMS < 实际执行时间
        maxTimeMS = 10;
        try {
            db.runCommand( new Document( "distinct", clName )
                    .append( "key", "a" ).append( "maxTimeMS", maxTimeMS ) );
        } catch ( MongoCommandException e ) {
            if ( -116 != e.getErrorCode() )
                throw e;
        }
    }
}
