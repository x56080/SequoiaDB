package com.mongodb.springdata;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.UUID;

import org.bson.types.ObjectId;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.gridfs.GridFsCriteria;
import org.springframework.data.mongodb.gridfs.GridFsTemplate;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DBObject;
import com.mongodb.gridfs.GridFSDBFile;
import com.mongodb.gridfs.GridFSFile;
import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;
import com.mongodb.utils.TestTools;

/**
 * @Description: seqDB-22021:创建文件
 * @author fanyu
 * @Date 2020/4/8
 * @version 1.00
 */
public class GridFS22021 extends MongodbTestBase {
    private GridFsTemplate gridFsTemplate1;
    private GridFsTemplate gridFsTemplate2;
    private byte[] bytes;
    private String filNameBase = "fs22021";
    private File localPath;
    private String bucketName;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        localPath = new File( File.separator + "tmp" + File.separator
                + File.separator + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );

        bucketName = springDBNameWithVersion + "_bucket22021";

        // 默认桶
        gridFsTemplate1 = new GridFsTemplate( mongoDbFactory, converter );
        // 自定义桶
        gridFsTemplate2 = new GridFsTemplate( mongoDbFactory, converter,
                bucketName );
        bytes = new byte[ 1024 ];
        new Random().nextBytes( bytes );
    }

    @Test
    public void test1() throws IOException {
        byte[] bytes1 = new byte[ 0 ];
        // 创建文件
        String fileName = filNameBase + "test1";
        GridFSFile gridFSFile = gridFsTemplate1.store(
                new ByteArrayInputStream( bytes1 ), filNameBase + "test1" );
        gridFSFile.save();

        // 获取文件
        Query query = new Query(
                GridFsCriteria.whereFilename().is( fileName ) );
        GridFSDBFile file = gridFsTemplate1.findOne( query );
        String downloadPath = localPath + File.separator + UUID.randomUUID();
        file.writeTo( new FileOutputStream( new File( downloadPath ) ) );
        // 检查
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( bytes1 ) );
        Assert.assertEquals( file.getFilename(), fileName );
        Assert.assertEquals( file.getAliases(), null );
        Assert.assertNull( file.getMetaData() );
        Assert.assertEquals( file.getContentType(), null );
        Assert.assertEquals( file.getLength(), 0 );
        Assert.assertEquals( file.getChunkSize(), 261120 );
        Assert.assertNotNull( file.getUploadDate() );
        Assert.assertEquals( file.getMD5(), TestTools.getMD5( bytes1 ) );
    }

    @Test
    public void test2() throws IOException {
        // 默认桶且存在，GridFS文件不存在，文件大小小于chunkSize，创建文件的方式为字节数组,有自定义元数据
        // 创建文件
        String fileName = filNameBase + "test2";
        GridFSFile gridFSFile = gridFsTemplate1
                .store( new ByteArrayInputStream( bytes ), fileName, "gif" );
        gridFSFile.put( "tag", "test2" );
        DBObject metadata = new BasicDBObject( "a", 1 ).append( "b", 2 );
        gridFSFile.setMetaData( metadata );
        gridFSFile.save();

        // 获取文件
        Query query = new Query( GridFsCriteria.whereFilename().is( fileName )
                .and( "contentType" ).is( "gif" ) );
        GridFSDBFile file = gridFsTemplate1.findOne( query );
        String downloadPath = localPath + File.separator + UUID.randomUUID();
        file.writeTo( new File( downloadPath ) );
        // 检查
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( bytes ) );
        Assert.assertEquals( file.getFilename(), fileName );
        Assert.assertEquals( file.getAliases(), null );
        Assert.assertEquals( file.getMetaData(), metadata );
        Assert.assertEquals( file.getContentType(), "gif" );
        Assert.assertEquals( file.getLength(), bytes.length );
        Assert.assertEquals( file.getChunkSize(), 261120 );
        Assert.assertNotNull( file.getUploadDate() );
        Assert.assertEquals( file.getMD5(), TestTools.getMD5( bytes ) );
        Assert.assertEquals( file.get( "tag" ), "test2" );
    }

    @Test
    public void test3() throws IOException {
        // 默认桶且存在，GridFS文件不存在，文件大小等于chunkSize，创建文件的方式为输入流,有自定义元数据
        byte[] bytes = new byte[ 261120 ];
        new Random().nextBytes( bytes );
        InputStream inputStream = new ByteArrayInputStream( bytes );
        String fileName = filNameBase + "test3";
        DBObject metadata = new BasicDBObject( "contentType", "txt" )
                .append( "tag", "test3" );
        // 创建文件
        gridFsTemplate1.store( inputStream, fileName, metadata );
        inputStream.close();

        // 获取文件
        Query query = new Query( GridFsCriteria.whereMetaData().is( metadata )
                .and( "filename" ).is( fileName ) );
        GridFSDBFile file = gridFsTemplate1.findOne( query );
        String downloadPath = localPath + File.separator + UUID.randomUUID();
        file.writeTo( downloadPath );
        // 检查
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( bytes ) );
        Assert.assertEquals( file.getFilename(), fileName );
        Assert.assertEquals( file.getAliases(), null );
        Assert.assertEquals( file.getMetaData(), metadata );
        Assert.assertEquals( file.getContentType(), null );
        Assert.assertEquals( file.getLength(), bytes.length );
        Assert.assertEquals( file.getChunkSize(), 261120 );
        Assert.assertNotNull( file.getUploadDate() );
        Assert.assertEquals( file.getMD5(), TestTools.getMD5( bytes ) );
    }

    @Test
    public void test4() throws IOException {
        // 非默认桶且不存在，GridFS文件不存在，文件大小大于chunkSize，创建文件的方式为输入流,有自定义元数据
        byte[] bytes = new byte[ 261121 ];
        new Random().nextBytes( bytes );
        InputStream inputStream = new ByteArrayInputStream( bytes );
        String fileName = filNameBase + "test4";
        Entity metadata = new Entity( "test4", "m", 3, 1, Entity.COURSES );
        // 创建文件
        gridFsTemplate2.store( inputStream, fileName, metadata );
        inputStream.close();
        // 获取文件
        Query query = new Query(
                GridFsCriteria.whereFilename().is( fileName ) );
        GridFSDBFile file = gridFsTemplate2.findOne( query );
        String downloadPath = localPath + File.separator + UUID.randomUUID();
        OutputStream outputStream = new FileOutputStream(
                new File( downloadPath ) );
        InputStream inputStream1 = file.getInputStream();
        try {
            byte[] read_buf = new byte[ 1024 ];
            int read_len;
            while ( ( read_len = inputStream1.read( read_buf ) ) > -1 ) {
                outputStream.write( read_buf, 0, read_len );
            }
        } finally {
            outputStream.close();
            inputStream.close();
        }
        // 检查
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( bytes ) );
        Assert.assertEquals( file.getFilename(), fileName );
        Assert.assertEquals( file.getAliases(), null );
        DBObject object = file.getMetaData();
        object.removeField( "_class" );
        Assert.assertEquals( object, metadata.toBSON().append( "_id", null ) );
        Assert.assertEquals( file.getContentType(), null );
        Assert.assertEquals( file.getLength(), bytes.length );
        Assert.assertEquals( file.getChunkSize(), 261120 );
        Assert.assertNotNull( file.getUploadDate() );
        Assert.assertEquals( file.getMD5(), TestTools.getMD5( bytes ) );
    }

    @Test
    public void test5() throws IOException {
        // 非默认桶且不存在，GridFS文件不存在，文件大小大于chunkSize，创建文件的方式为输入流,有自定义元数据
        String filePath = localPath + File.separator + "test5";
        TestTools.LocalFile.createFile( filePath, 1024 * 1024 * 50 );
        InputStream inputStream = new FileInputStream( filePath );
        String fileName = filNameBase + "test5";
        // 创建文件
        DBObject metadata = new BasicDBObject( "tag", "test5" );
        gridFsTemplate2.store( inputStream, fileName, "pdf", metadata );
        inputStream.close();
        // 获取文件
        Query query = new Query( GridFsCriteria.whereMetaData().is( metadata )
                .and( "filename" ).is( fileName ) );
        GridFSDBFile file = gridFsTemplate2.findOne( query );
        String downloadPath = localPath + File.separator + UUID.randomUUID();
        OutputStream outputStream = null;
        try {
            outputStream = new FileOutputStream( new File( downloadPath ) );
            InputStream inputStream1 = file.getInputStream();
            byte[] read_buf = new byte[ 1024 ];
            int read_len;
            while ( ( read_len = inputStream1.read( read_buf ) ) > -1 ) {
                outputStream.write( read_buf, 0, read_len );
            }
        } finally {
            if ( outputStream != null ) {
                outputStream.close();
            }

        }

        // 检查
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( downloadPath ) );
        Assert.assertEquals( file.getFilename(), fileName );
        Assert.assertEquals( file.getAliases(), null );
        DBObject object = file.getMetaData();
        Assert.assertEquals( object, metadata );
        Assert.assertEquals( file.getContentType(), "pdf" );
        Assert.assertEquals( file.getLength(), new File( filePath ).length() );
        Assert.assertEquals( file.getChunkSize(), 261120 );
        Assert.assertNotNull( file.getUploadDate() );
        Assert.assertEquals( file.getMD5(), TestTools.getMD5( filePath ) );
    }

    @Test
    public void test6() throws IOException {
        // 非默认桶且不存在，GridFS文件不存在，文件大小小于chunkSize，创建文件的方式为输入流,有自定义元数据
        byte[] bytes = new byte[ 261120 ];
        new Random().nextBytes( bytes );
        InputStream inputStream = new ByteArrayInputStream( bytes );
        String fileName = filNameBase + "test6";
        Entity metadata = new Entity( "test6", "m", 3, 1, Entity.COURSES );
        // 创建文件
        GridFSFile gridFSFile = gridFsTemplate2.store( inputStream, fileName,
                metadata );
        gridFSFile.put( "chunkSize", 1024 * 1024 );
        gridFSFile.save();
        // 获取文件
        Query query = new Query(
                GridFsCriteria.whereFilename().is( fileName ) );
        GridFSDBFile file = gridFsTemplate2.findOne( query );
        String downloadPath = localPath + File.separator + UUID.randomUUID();
        file.writeTo( new File( downloadPath ) );
        // 检查
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( bytes ) );
        Assert.assertEquals( file.getFilename(), fileName );
        Assert.assertEquals( file.getAliases(), null );
        DBObject object = file.getMetaData();
        object.removeField( "_class" );
        Assert.assertEquals( object, metadata.toBSON().append( "_id", null ) );
        Assert.assertEquals( file.getContentType(), null );
        Assert.assertEquals( file.getLength(), bytes.length );
        Assert.assertEquals( file.getChunkSize(), 1024 * 1024 );
        Assert.assertNotNull( file.getUploadDate() );
        Assert.assertEquals( file.getMD5(), TestTools.getMD5( bytes ) );
    }

    @Test
    public void test7() throws IOException {
        // 重复创建文件，文件名相同，文件内容不同，文件id不同
        List< byte[] > bytesList = new ArrayList<>();
        Random random = new Random();
        for ( int i = 0; i < 2; i++ ) {
            byte[] bytes = new byte[ 1024 - i ];
            random.nextBytes( bytes );
            bytesList.add( bytes );
        }

        // 创建文件
        String fileName = filNameBase + "test7";
        List< String > idList = new ArrayList<>();
        for ( byte[] bytes : bytesList ) {
            GridFSFile gridFSFile = gridFsTemplate1
                    .store( new ByteArrayInputStream( bytes ), fileName );
            idList.add( gridFSFile.getId().toString() );
        }

        // 获取文件
        for ( int i = 0; i < idList.size(); i++ ) {
            // 获取文件
            Query query = new Query( GridFsCriteria.where( "_id" )
                    .is( new ObjectId( idList.get( i ) ) ) );
            GridFSDBFile file = gridFsTemplate1.findOne( query );
            String downloadPath = localPath + File.separator
                    + UUID.randomUUID();
            file.writeTo( new File( downloadPath ) );
            // 检查
            Assert.assertEquals( TestTools.getMD5( downloadPath ),
                    TestTools.getMD5( bytesList.get( i ) ) );
            Assert.assertEquals( file.getFilename(), fileName );
            Assert.assertEquals( file.getAliases(), null );
            Assert.assertNull( file.getMetaData() );
            Assert.assertEquals( file.getContentType(), null );
            Assert.assertEquals( file.getLength(), bytesList.get( i ).length );
            Assert.assertEquals( file.getChunkSize(), 261120 );
            Assert.assertNotNull( file.getUploadDate() );
            Assert.assertEquals( file.getMD5(),
                    TestTools.getMD5( bytesList.get( i ) ) );
        }
    }

    @Test
    public void test8() throws IOException {
        // 重复创建文件，文件Id相同
        List< byte[] > bytesList = new ArrayList<>();
        Random random = new Random();
        for ( int i = 0; i < 2; i++ ) {
            byte[] bytes = new byte[ 1024 - i ];
            random.nextBytes( bytes );
            bytesList.add( bytes );
        }
        List< String > idList = new ArrayList<>();
        String fileName = filNameBase + "test8";
        GridFSFile gridFSFile1 = gridFsTemplate1.store(
                new ByteArrayInputStream( bytesList.get( 0 ) ), fileName );
        gridFSFile1.put( "version", 0 );
        gridFSFile1.save();
        String fileId = gridFSFile1.getId().toString();
        idList.add( fileId );

        // spring data 不让设置_id,所以会成功
        GridFSFile gridFSFile2 = gridFsTemplate1.store(
                new ByteArrayInputStream( bytesList.get( 1 ) ), fileName );
        gridFSFile2.put( "version", 1 );
        gridFSFile2.save();
        idList.add( gridFSFile2.getId().toString() );
        // 获取文件
        for ( int i = 0; i < idList.size(); i++ ) {
            // 获取文件
            Query query = new Query( GridFsCriteria.where( "_id" )
                    .is( new ObjectId( idList.get( i ) ) ).and( "version" )
                    .is( i ) );
            GridFSDBFile file = gridFsTemplate1.findOne( query );
            String downloadPath = localPath + File.separator
                    + UUID.randomUUID();
            file.writeTo( new File( downloadPath ) );
            // 检查
            Assert.assertEquals( TestTools.getMD5( downloadPath ),
                    TestTools.getMD5( bytesList.get( i ) ) );
            Assert.assertEquals( file.getFilename(), fileName );
            Assert.assertEquals( file.getAliases(), null );
            Assert.assertNull( file.getMetaData() );
            Assert.assertEquals( file.getContentType(), null );
            Assert.assertEquals( file.getLength(), bytesList.get( i ).length );
            Assert.assertEquals( file.getChunkSize(), 261120 );
            Assert.assertNotNull( file.getUploadDate() );
            Assert.assertEquals( file.getMD5(),
                    TestTools.getMD5( bytesList.get( i ) ) );
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = { bucketName + ".files", bucketName + ".chunks",
                "fs.files", "fs.chunks" };
        dropCLByTestResult( context, this.toString(), mongoTemplate, clNames );
        TestTools.LocalFile.removeFile( localPath );
    }
}
