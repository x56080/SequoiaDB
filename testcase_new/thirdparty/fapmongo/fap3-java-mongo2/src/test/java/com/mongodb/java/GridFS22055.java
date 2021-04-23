package com.mongodb.java;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.Set;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBObject;
import com.mongodb.gridfs.GridFS;
import com.mongodb.gridfs.GridFSInputFile;
import com.mongodb.util.JSON;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-22055:存在gridfs的集合下，列取索引/列取集合/删除集合
 * @author fanyu
 * @Date 2020/4/9
 * @version 1.00
 */
public class GridFS22055 extends MongodbTestBase {
    private DB db;
    private String bucketName;
    private String[] expClNames = new String[ 2 ];

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        bucketName = javaDBNameWithVersion + "_bucket22055";
        expClNames[ 0 ] = bucketName + "." + "chunks";
        expClNames[ 1 ] = bucketName + "." + "files";
    }

    @Test
    public void test1() throws IOException {
        // 集合存在文件，使用查询条件删除文件
        GridFS gridFS = new GridFS( db, bucketName );
        // 创建文件，使gridFS对应的集合存在
        byte[] bytes = new byte[ 1024 ];
        new Random().nextBytes( bytes );
        String fileName = "fs22055";
        GridFSInputFile file = gridFS.createFile( bytes );
        file.setFilename( fileName );
        file.save();

        // 列取集合名
        Set< String > clNames = db.getCollectionNames();
        Assert.assertTrue( clNames.size() >= expClNames.length );
        Assert.assertTrue( clNames.containsAll( Arrays.asList( expClNames ) ),
                "clNames = " + clNames.toString() );

        // 列取索引，默认存在2个索引
        DBCollection cl1 = db.getCollection( expClNames[ 0 ] );
        List< DBObject > indexInfo1 = cl1.getIndexInfo();
        Assert.assertEquals( indexInfo1.size(), 2, indexInfo1.toString() );
        Assert.assertEquals( indexInfo1.get( 0 ).get( "key" ),
                JSON.parse( "{ \"_id\" : 1}" ) );
        Assert.assertEquals( indexInfo1.get( 0 ).get( "ns" ),
                db.getName() + "." + expClNames[ 0 ] );

        DBCollection cl2 = db.getCollection( expClNames[ 1 ] );
        List< DBObject > indexInfo2 = cl2.getIndexInfo();
        Assert.assertEquals( indexInfo2.size(), 2, indexInfo1.toString() );
        Assert.assertEquals( indexInfo2.get( 0 ).get( "key" ),
                JSON.parse( "{ \"_id\" : 1}" ) );
        Assert.assertEquals( indexInfo2.get( 0 ).get( "ns" ),
                db.getName() + "." + expClNames[ 1 ] );

        // 创建索引
        // 先分别删除一个索引
        cl1.dropIndex( "files_id_1_n_1" );
        cl2.dropIndex( "filename_1_uploadDate_1" );
        indexInfo1 = cl1.getIndexInfo();
        indexInfo2 = cl2.getIndexInfo();
        Assert.assertEquals( indexInfo1.size(), 1, indexInfo1.toString() );
        Assert.assertEquals( indexInfo2.size(), 1, indexInfo2.toString() );
        // 再次创建索引
        cl1.createIndex( new BasicDBObject( "files_id", 1 ).append( "n", 1 ) );
        cl2.createIndex(
                new BasicDBObject( "filename", 1 ).append( "uploadDate", 1 ) );
        indexInfo1 = cl1.getIndexInfo();
        indexInfo2 = cl2.getIndexInfo();
        Assert.assertEquals( indexInfo1.size(), 2, indexInfo1.toString() );
        Assert.assertEquals( indexInfo2.size(), 2, indexInfo2.toString() );

        // count集合
        Assert.assertEquals( cl1.count(), 1 );
        Assert.assertEquals( cl2.count(), 1 );
        cl1.remove( new BasicDBObject() );
        cl2.remove( new BasicDBObject() );
        Assert.assertEquals( cl1.count(), 0 );
        Assert.assertEquals( cl2.count(), 0 );

        // 删除集合名
        for ( String clName : expClNames ) {
            db.getCollection( clName ).drop();
        }
        Set< String > clNames1 = db.getCollectionNames();
        Assert.assertFalse( clNames1.containsAll( Arrays.asList( expClNames ) ),
                "clNames1 = " + clNames1.toString() );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, expClNames );
    }
}
