package com.mongodb.java;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.UUID;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.gridfs.GridFS;
import com.mongodb.gridfs.GridFSDBFile;
import com.mongodb.gridfs.GridFSInputFile;
import com.mongodb.utils.MongodbTestBase;
import com.mongodb.utils.TestTools;

/**
 * @Description: seqDB-22021:创建文件
 * @author fanyu
 * @Date:2020/4/8
 * @version:1.0
 */
public class GridFS22021 extends MongodbTestBase {
    private DB db;
    private File localPath;
    private String filNameBase = "fs22021";
    private String bucketName;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        localPath = new File( File.separator + "tmp" + File.separator
                + File.separator + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        db = MongodbTestBase.getDB( client );
        bucketName = javaDBNameWithVersion + "_bucket22021";
    }

    @Test
    public void test1() throws IOException {
        // 默认桶且不存在，GridFS文件不存在，文件大小为0，创建文件的方式为文件实例,无自定义元数据
        String filePath = localPath + File.separator + filNameBase + "_test1";
        TestTools.LocalFile.createFile( filePath, 0 );
        GridFS gridFS = new GridFS( db );

        // 创建文件
        GridFSInputFile gfile = gridFS.createFile( new File( filePath ) );
        gfile.save();
        String fileId = gfile.getId().toString();

        // 通过文件id获取文件,检查文件元数据及文件内容
        GridFSDBFile file1 = gridFS.find( new ObjectId( fileId ) );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", 0 )
                .append( "md5", TestTools.getMD5( filePath ) )
                .append( "filename", new File( filePath ).getName() )
                .append( "contentType", null ).append( "aliases", null )
                .append( "uploadDate", null ).append( "_id", file1.getId() );
        checkFindResults( file1, exp );
    }

    @Test
    public void test2() throws IOException {
        // 默认桶且存在，GridFS文件不存在，文件大小小于chunkSize，创建文件的方式为字节数组,有自定义元数据
        GridFS gridFS = new GridFS( db );
        byte[] bytes = new byte[ 261119 ];
        new Random().nextBytes( bytes );

        // 创建文件
        GridFSInputFile gfile = gridFS.createFile( bytes );
        gfile.put( "a", "1" );
        gfile.setMetaData( new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        gfile.save();
        // 检查
        String fileId = gfile.getId().toString();
        GridFSDBFile file1 = gridFS.find( new ObjectId( fileId ) );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytes.length )
                .append( "md5", TestTools.getMD5( bytes ) )
                .append( "filename", null ).append( "contentType", null )
                .append( "aliases", null ).append( "uploadDate", null )
                .append( "_id", file1.getId() ).append( "a", "1" )
                .append( "metadata",
                        new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        checkFindResults( file1, exp );
    }

    @Test
    public void test3() throws IOException {
        // 默认桶且存在，GridFS文件不存在，文件大小等于chunkSize，创建文件的方式为输入流,有自定义元数据
        GridFS gridFS = new GridFS( db );
        byte[] bytes = new byte[ 261120 ];
        new Random().nextBytes( bytes );
        InputStream inputStream = new ByteArrayInputStream( bytes );

        // 创建文件
        String fileName = filNameBase + "_test3";
        gridFS.remove( new BasicDBObject( "filename", fileName ) );
        GridFSInputFile gfile = gridFS.createFile( inputStream );
        gfile.setFilename( fileName );
        gfile.setContentType( "txt" );
        String id = UUID.randomUUID().toString();
        gfile.setId( id );
        gfile.put( "a", "1" );
        gfile.setMetaData( new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        gfile.save();
        // 检查
        List< GridFSDBFile > file1s = gridFS.find( fileName );
        Assert.assertEquals( file1s.size(), 1 );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytes.length )
                .append( "md5", TestTools.getMD5( bytes ) )
                .append( "filename", fileName ).append( "contentType", "txt" )
                .append( "aliases", null ).append( "uploadDate", null )
                .append( "_id", id ).append( "a", "1" ).append( "metadata",
                        new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        checkFindResults( file1s.get( 0 ), exp );
    }

    @Test
    public void test4() throws IOException {
        // 非默认桶且不存在，GridFS文件不存在，文件大小大于chunkSize，创建文件的方式为输入流,有自定义元数据
        GridFS gridFS = new GridFS( db );
        byte[] bytes = new byte[ 261121 ];
        new Random().nextBytes( bytes );
        InputStream inputStream = new ByteArrayInputStream( bytes );

        String fileName = filNameBase + "_test4";
        // 创建文件
        GridFSInputFile gfile = gridFS.createFile( inputStream, true );
        gfile.setFilename( fileName );
        gfile.setContentType( "jpg" );
        String id = UUID.randomUUID().toString();
        gfile.setId( id );
        gfile.put( "a", "1" );
        gfile.setMetaData( new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        gfile.save();
        List< GridFSDBFile > file1s = gridFS.find( fileName );
        Assert.assertEquals( file1s.size(), 1 );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytes.length )
                .append( "md5", TestTools.getMD5( bytes ) )
                .append( "filename", fileName ).append( "contentType", "jpg" )
                .append( "aliases", null ).append( "uploadDate", null )
                .append( "_id", id ).append( "a", "1" ).append( "metadata",
                        new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        checkFindResults( file1s.get( 0 ), exp );
    }

    @Test
    public void test5() throws IOException {
        // 非默认桶且不存在，GridFS文件不存在，文件大小大于chunkSize，创建文件的方式为输入流,有自定义元数据
        GridFS gridFS = new GridFS( db, bucketName );
        byte[] bytes = new byte[ 261120 * 10 ];
        new Random().nextBytes( bytes );
        InputStream inputStream = new ByteArrayInputStream( bytes );

        String fileName = filNameBase + "_test5";
        // 创建文件
        GridFSInputFile gfile = gridFS.createFile( inputStream, fileName );
        gfile.setContentType( "jpg" );
        String id = UUID.randomUUID().toString();
        gfile.setId( id );
        gfile.setChunkSize( 261120 * 2 );
        gfile.put( "a", "1" );
        gfile.setMetaData( new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        gfile.save();
        List< GridFSDBFile > file1s = gridFS.find( fileName );
        Assert.assertEquals( file1s.size(), 1 );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 * 2 )
                .append( "length", bytes.length )
                .append( "md5", TestTools.getMD5( bytes ) )
                .append( "filename", fileName ).append( "contentType", "jpg" )
                .append( "aliases", null ).append( "uploadDate", null )
                .append( "_id", id ).append( "a", "1" ).append( "metadata",
                        new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        checkFindResults( file1s.get( 0 ), exp );
    }

    @Test
    public void test6() throws IOException {
        // 非默认桶且不存在，GridFS文件不存在，文件大小大于chunkSize，创建文件的方式为输入流,有自定义元数据
        GridFS gridFS = new GridFS( db, bucketName );
        String filePath = localPath + File.separator + filNameBase + "_test6";
        TestTools.LocalFile.createFile( filePath, 1024 * 1024 * 200 );
        InputStream inputStream = new FileInputStream( new File( filePath ) );

        String fileName = filNameBase + "_test6";
        // 创建文件
        GridFSInputFile gfile = gridFS.createFile( inputStream, fileName,
                true );
        gfile.setContentType( "jpg" );
        gfile.setChunkSize( 1024 * 1024 );
        gfile.put( "a", "1" );
        gfile.setMetaData( new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        gfile.save();
        String fileId = gfile.getId().toString();
        // 检查
        GridFSDBFile file1 = gridFS.find( new ObjectId( fileId ) );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 1024 * 1024 )
                .append( "length", new File( filePath ).length() )
                .append( "md5", TestTools.getMD5( filePath ) )
                .append( "filename", fileName ).append( "contentType", "jpg" )
                .append( "aliases", null ).append( "uploadDate", null )
                .append( "_id", file1.getId() ).append( "a", "1" )
                .append( "metadata",
                        new BasicDBObject( "b", "2" ).append( "c", 3 ) );
        checkFindResults( file1, exp );
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

        GridFS gridFS = new GridFS( db );
        String fileName = filNameBase + "_test7";
        gridFS.remove( new BasicDBObject( "filename", fileName ) );
        // 创建文件
        List< String > idList = new ArrayList<>();
        for ( int i = 0; i < bytesList.size(); i++ ) {
            GridFSInputFile gfile = gridFS.createFile( bytesList.get( i ) );
            gfile.setFilename( fileName );
            gfile.save();
            idList.add( gfile.getId().toString() );
        }

        // 检查
        for ( int i = 0; i < idList.size(); i++ ) {
            GridFSDBFile file1 = gridFS.find( new ObjectId( idList.get( i ) ) );
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length", bytesList.get( i ).length )
                    .append( "md5", TestTools.getMD5( bytesList.get( i ) ) )
                    .append( "filename", fileName )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null )
                    .append( "_id", file1.getId() );
            checkFindResults( file1, exp );
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

        GridFS gridFS = new GridFS( db );
        String fileName = filNameBase + "_testk8";
        gridFS.remove( new BasicDBObject( "filename", fileName ) );
        String id = TestTools.getRandomString( 24 );
        // 创建文件
        GridFSInputFile gfile = gridFS.createFile( bytesList.get( 0 ) );
        gfile.setFilename( fileName );
        gfile.setId( id );
        gfile.save();

        // 重复创建
        GridFSInputFile gfile1 = gridFS.createFile( bytesList.get( 1 ) );
        gfile1.setFilename( fileName );
        gfile1.setId( id );
        try {
            gfile1.save();
            Assert.fail( "exp fail but act success!!!" );
        } catch ( Exception e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }

        // 检查
        List< GridFSDBFile > files = gridFS.find( fileName );
        Assert.assertEquals( files.size(), 1 );
        GridFSDBFile file1 = files.get( 0 );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytesList.get( 0 ).length )
                .append( "md5", TestTools.getMD5( bytesList.get( 0 ) ) )
                .append( "filename", fileName ).append( "contentType", null )
                .append( "aliases", null ).append( "uploadDate", null )
                .append( "_id", file1.getId() );
        checkFindResults( file1, exp );
    }

    private void checkFindResults( GridFSDBFile act, BasicDBObject exp )
            throws IOException {
        Set< String > actKeySet = act.keySet();
        Set< String > expKeySet = exp.keySet();
        Assert.assertEquals( actKeySet, expKeySet );
        Assert.assertEquals( act.getFilename(), exp.get( "filename" ) );
        Assert.assertEquals( act.getChunkSize(), exp.getInt( "chunkSize" ) );
        Assert.assertEquals( act.getMD5(), exp.get( "md5" ) );
        Assert.assertEquals( act.getContentType(), exp.get( "contentType" ) );
        Assert.assertEquals( act.getAliases(), exp.get( "aliases" ) );
        Assert.assertNotNull( act.getUploadDate() );
        Assert.assertEquals( act.getMetaData(), exp.get( "metadata" ) );
        Assert.assertEquals( act.getLength(), exp.getLong( "length" ) );
        Assert.assertNotNull( act.getId() );
        Assert.assertEquals( act.getId(), exp.get( "_id" ) );

        String downloadPath = localPath + File.separator + UUID.randomUUID();
        act.writeTo( downloadPath );
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                exp.get( "md5" ) );

        expKeySet.removeAll( Arrays.asList( "filename", "md5", "contentType",
                "aliases", "chunkSize", "uploadDate", "metadata", "length",
                "_id" ) );
        for ( String key : expKeySet ) {
            Assert.assertEquals( act.get( key ), exp.get( key ),
                    "expKeySet = " + expKeySet );
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = { bucketName + ".files", bucketName + ".chunks",
                "fs.files", "fs.chunks" };
        dropCLByTestResult( context, this.toString(), db, clNames );
        TestTools.LocalFile.removeFile( localPath );
    }
}
