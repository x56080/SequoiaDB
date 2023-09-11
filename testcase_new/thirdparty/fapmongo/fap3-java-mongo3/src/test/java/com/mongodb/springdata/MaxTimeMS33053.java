package com.mongodb.springdata;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.concurrent.TimeUnit;

import org.bson.Document;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.UncategorizedMongoDbException;
import org.springframework.data.mongodb.core.IndexOperations;
import org.springframework.data.mongodb.core.index.IndexInfo;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.CommandResult;
import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33053:find/createIndexes操作时使用maxTimeMS参数
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class MaxTimeMS33053 extends MongodbTestBase {
    private String clName;
    private int docsNum = 50000;

    @BeforeClass
    private void setUp() {
        clName = springDBNameWithVersion + "_cl_33053";
        mongoTemplate.dropCollection( clName );
        mongoTemplate.createCollection( clName );

        Random random = new Random();
        List< Document > docs = new ArrayList<>();
        for ( int i = 0; i < docsNum; i++ ) {
            docs.add( new Document( "a", i ).append( "b", docsNum - i - 1 )
                    .append( "c", random.nextInt( docsNum ) ) );
        }
        mongoTemplate.insert( docs, clName );
    }

    @Test
    private void test() {
        this.testFind();
        this.testCreateIndexes();
        this.testDistinct();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }

    private void testFind() {
        long maxTimeMS;
        Query query;

        // maxTimeMS 大于实际执行时间
        maxTimeMS = 10 * 1000;
        query = new Query( Criteria.where( "a" ).gte( 100 ) )
                .with( new Sort( Sort.Direction.DESC, "a" ) )
                .maxTime( maxTimeMS, TimeUnit.MILLISECONDS );
        List< Entity > actDocs = mongoTemplate.find( query, Entity.class,
                clName );
        Assert.assertEquals( actDocs.size(), docsNum - 100 );

        // maxTimeMS < 实际执行时间
        maxTimeMS = 10;
        query = new Query( Criteria.where( "a" ).gte( 100 ) )
                .with( new Sort( Sort.Direction.DESC, "a" ) )
                .maxTime( maxTimeMS, TimeUnit.MILLISECONDS );
        try {
            mongoTemplate.find( query, Entity.class, clName );
        } catch ( UncategorizedMongoDbException e ) {
            System.out.println( e.getMessage() );
            if ( !e.getMessage().contains( "Operation exceeded time limit" ) )
                throw e;
        }
    }

    private void testCreateIndexes() {
        long maxTimeMS;
        List< String > indexNames = Arrays.asList( "idx1", "idx2", "idx3" );
        // 索引定义
        IndexOperations idxOpt = mongoTemplate.indexOps( clName );
        List< Document > indexes = new ArrayList<>();
        indexes.add( new Document( "key", new Document( "a", -1 ) )
                .append( "name", indexNames.get( 0 ) ) );
        indexes.add( new Document( "key", new Document( "b", 1 ) )
                .append( "name", indexNames.get( 1 ) ) );
        indexes.add( new Document( "key", new Document( "c", -1 ) )
                .append( "name", indexNames.get( 2 ) ) );

        // maxTimeMS 大于实际执行时间
        maxTimeMS = 10 * 1000;
        mongoTemplate
                .executeCommand( new BasicDBObject( "createIndexes", clName )
                        .append( "indexes", indexes )
                        .append( "maxTimeMS", maxTimeMS ) );

        List< IndexInfo > actIndexes = idxOpt.getIndexInfo();
        Assert.assertEquals( actIndexes.size(), indexes.size() + 1 );
        for ( String name : indexNames )
            idxOpt.dropIndex( name );

        // maxTimeMS < 实际执行时间
        maxTimeMS = 1;
        try {
            mongoTemplate.executeCommand(
                    new BasicDBObject( "createIndexes", clName )
                            .append( "indexes", indexes )
                            .append( "maxTimeMS", maxTimeMS ) );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "Operation exceeded time limit" ) )
                throw e;
        }
    }

    private void testDistinct() {
        long maxTimeMS;

        // maxTimeMS 大于实际执行时间
        maxTimeMS = 10 * 1000;
        CommandResult actDocs = mongoTemplate.executeCommand(
                new BasicDBObject( "distinct", clName ).append( "key", "a" )
                        .append( "maxTimeMS", maxTimeMS ) );
        @SuppressWarnings("unchecked")
        List< Integer > rcDocsNum = ( ArrayList< Integer > ) actDocs
                .get( "values" );
        Assert.assertEquals( rcDocsNum.size(), docsNum );

        // maxTimeMS < 实际执行时间
        maxTimeMS = 1;
        try {
            mongoTemplate
                    .executeCommand( new BasicDBObject( "distinct", clName )
                            .append( "key", "a" ).append( "key", "b" )
                            .append( "maxTimeMS", maxTimeMS ) );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "Operation exceeded time limit" ) )
                throw e;
        }
    }
}
