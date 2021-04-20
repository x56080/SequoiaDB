package com.mongodb.java;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBObject;
import com.mongodb.QueryBuilder;
import com.mongodb.gridfs.GridFS;
import com.mongodb.gridfs.GridFSInputFile;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-22023:删除文件
 * @author fanyu
 * @Date 2020/4/9
 * @version 1.00
 */
public class GridFS22023 extends MongodbTestBase {
    private DB db;
    private String fileNameBase = "fs22023";
    private String bucketName1 = javaDBNameWithVersion + "_bucket22023A";
    private String bucketName2 = javaDBNameWithVersion + "_bucket22023B";
    private byte[] bytes = new byte[ 1024 ];

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDB( client );
        new Random().nextBytes( bytes );
    }

    @Test
    public void test1() throws IOException {
        // 集合存在文件，使用查询条件删除文件
        GridFS gridFS = new GridFS( db, bucketName1 );
        // 创建文件
        String fileName = fileNameBase + "test1";
        int fileNum = 10;
        List< String > fileIdList = new ArrayList<>();
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSInputFile gridFSInputFile = gridFS.createFile( bytes );
            gridFSInputFile.setFilename( fileName );
            gridFSInputFile.save();
            fileIdList.add( gridFSInputFile.getId().toString() );
        }
        // 使用文件id删除文件
        gridFS.remove( new ObjectId( fileIdList.get( 0 ) ) );
        Assert.assertNull( gridFS.find( new ObjectId( fileIdList.get( 0 ) ) ) );
        Assert.assertEquals( gridFS.getFileList().size(), fileNum - 1 );

        // 使用不存在的文件id删除文件
        gridFS.remove( new ObjectId( fileIdList.get( 0 ) ) );
        Assert.assertEquals( gridFS.getFileList().size(), fileNum - 1 );

        // 使用不存在的文件名删除文件
        gridFS.remove( fileName + "-notexist" );
        Assert.assertEquals( gridFS.getFileList().size(), fileNum - 1 );

        // 使用文件名删除文件
        gridFS.remove( fileName );
        Assert.assertEquals( gridFS.getFileList().size(), 0 );

        // 集合不存在文件删除文件
        gridFS.remove( fileName );
        Assert.assertEquals( gridFS.getFileList().size(), 0 );
    }

    @Test
    public void test2() throws IOException {
        // 集合存在文件，使用查询条件删除文件
        GridFS gridFS = new GridFS( db, bucketName2 );
        // 创建文件
        int fileNum = 10;
        List< String > fileIdList = new ArrayList<>();
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSInputFile gridFSInputFile = gridFS.createFile( bytes );
            gridFSInputFile.setFilename( fileNameBase + i );
            gridFSInputFile.put( "a", i );
            gridFSInputFile.save();
            fileIdList.add( gridFSInputFile.getId().toString() );
        }
        // 使用查询条件删除文件
        DBObject query1 = QueryBuilder.start( "a" ).greaterThanEquals( 0 )
                .lessThan( fileNum / 2 ).get();
        Assert.assertEquals( gridFS.getFileList( query1 ).size(), fileNum / 2 );
        gridFS.remove( query1 );
        Assert.assertEquals( gridFS.getFileList( query1 ).size(), 0 );

        // 指定不存在的字段当查询条件删除文件
        DBObject query2 = QueryBuilder.start( "b" )
                .greaterThanEquals( fileNum / 2 ).lessThan( fileNum ).get();
        Assert.assertEquals( gridFS.getFileList( query2 ).size(), 0 );
        gridFS.remove( query2 );
        Assert.assertEquals( gridFS.getFileList().size(), fileNum / 2 );

        // 匹配不到文件，删除文件
        DBObject query3 = QueryBuilder.start( "a" ).greaterThanEquals( fileNum )
                .get();
        Assert.assertEquals( gridFS.getFileList( query3 ).size(), 0 );
        gridFS.remove( query3 );
        Assert.assertEquals( gridFS.getFileList().size(), fileNum / 2 );

        // 删除所有文件
        gridFS.remove( new BasicDBObject() );
        Assert.assertEquals( gridFS.getFileList().size(), 0 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = { bucketName1 + ".files", bucketName1 + ".chunks",
                bucketName2 + ".files", bucketName2 + ".chunks" };
        dropCLByTestResult( context, this.toString(), db, clNames );
    }
}
