package com.mongodb.java;

import java.io.File;
import java.io.IOException;
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
import com.mongodb.DBObject;
import com.mongodb.QueryBuilder;
import com.mongodb.gridfs.GridFS;
import com.mongodb.gridfs.GridFSDBFile;
import com.mongodb.gridfs.GridFSInputFile;
import com.mongodb.utils.MongodbTestBase;
import com.mongodb.utils.TestTools;

/**
 * @Description: seqDB-22022:查询文件
 * @author fanyu
 * @Date 2020/4/8
 * @version 1.00
 */
public class GridFS22022 extends MongodbTestBase {
    private DB db;
    private String fileNameBase = "fs22022-";
    private String bucketName1;
    private String bucketName2;
    private int fileNum = 20;
    private File localPath;
    private List< byte[] > bytesList = new ArrayList<>();
    private List< String > fileIdList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws UnknownHostException {
        localPath = new File( File.separator + "tmp" + File.separator
                + File.separator + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        db = MongodbTestBase.getDB( client );
        bucketName1 = javaDBNameWithVersion + "_bucket22022A";
        bucketName2 = javaDBNameWithVersion + "_bucket22022B";

        Random random = new Random();
        for ( int i = 0; i < Math.min( 5, fileNum ); i++ ) {
            byte[] bytes = new byte[ 1024 * 200 + i ];
            random.nextBytes( bytes );
            bytesList.add( bytes );
        }
        // 准备文件
        GridFS gridFS = new GridFS( db, bucketName1 );
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSInputFile gfile = gridFS
                    .createFile( bytesList.get( i % bytesList.size() ) );
            gfile.setFilename( fileNameBase + i % bytesList.size() );
            gfile.put( "flag", i % bytesList.size() );
            gfile.put( "a", i );
            gfile.put( "b", String.valueOf( i ) );
            gfile.setMetaData(
                    new BasicDBObject( "c", i + 1 ).append( "d", i + 2 ) );
            gfile.save();
            fileIdList.add( gfile.getId().toString() );
        }
    }

    @Test
    public void test1() throws IOException {
        // 使用文件id查询单个文件
        GridFS gridFS = new GridFS( db, bucketName1 );
        GridFSDBFile file = gridFS.find( new ObjectId( fileIdList.get( 0 ) ) );
        // 检查
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytesList.get( 0 ).length )
                .append( "md5", TestTools.getMD5( bytesList.get( 0 ) ) )
                .append( "filename", fileNameBase + "0" )
                .append( "contentType", null ).append( "aliases", null )
                .append( "uploadDate", null ).append( "_id", file.getId() )
                .append( "a", 0 ).append( "b", "0" ).append( "flag", 0 )
                .append( "metadata",
                        new BasicDBObject( "c", 1 ).append( "d", 2 ) );
        checkFindResults( file, exp );
    }

    @Test
    public void test2() throws IOException {
        // 使用文件名查询单个文件
        GridFS gridFS = new GridFS( db, bucketName1 );
        int index = new Random().nextInt( bytesList.size() );
        String fileName = fileNameBase + index;
        GridFSDBFile file = gridFS.findOne( fileName );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytesList.get( index ).length )
                .append( "md5", TestTools.getMD5( bytesList.get( index ) ) )
                .append( "filename", fileName ).append( "contentType", null )
                .append( "aliases", null ).append( "uploadDate", null )
                .append( "_id", file.getId() ).append( "a", file.get( "a" ) )
                .append( "b", String.valueOf( file.get( "b" ) ) )
                .append( "flag", index ).append( "metadata",
                        new BasicDBObject( "c", ( int ) file.get( "a" ) + 1 )
                                .append( "d", ( int ) file.get( "a" ) + 2 ) );
        checkFindResults( file, exp );
    }

    @Test
    public void test3() throws IOException {
        // 使用查询条件查询单个文件
        GridFS gridFS = new GridFS( db, bucketName1 );
        DBObject query = QueryBuilder.start( "a" )
                .greaterThanEquals( fileNum / 2 ).lessThan( fileNum ).get();
        GridFSDBFile file = gridFS.findOne( query );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length",
                        bytesList.get( ( int ) file.get( "flag" ) ).length )
                .append( "md5",
                        TestTools.getMD5(
                                bytesList.get( ( int ) file.get( "flag" ) ) ) )
                .append( "filename", fileNameBase + ( int ) file.get( "flag" ) )
                .append( "contentType", null ).append( "aliases", null )
                .append( "uploadDate", null ).append( "_id", file.getId() )
                .append( "a", file.get( "a" ) )
                .append( "b", String.valueOf( file.get( "b" ) ) )
                .append( "flag", file.get( "flag" ) ).append( "metadata",
                        new BasicDBObject( "c", ( int ) file.get( "a" ) + 1 )
                                .append( "d", ( int ) file.get( "a" ) + 2 ) );
        checkFindResults( file, exp );
    }

    @Test
    public void test4() throws IOException {
        // 使用文件id查询单个文件
        GridFS gridFS = new GridFS( db, bucketName1 );
        GridFSDBFile file = gridFS
                .findOne( new ObjectId( fileIdList.get( 0 ) ) );
        // 检查
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytesList.get( 0 ).length )
                .append( "md5", TestTools.getMD5( bytesList.get( 0 ) ) )
                .append( "filename", fileNameBase + "0" )
                .append( "contentType", null ).append( "aliases", null )
                .append( "uploadDate", null ).append( "_id", file.getId() )
                .append( "a", 0 ).append( "b", "0" ).append( "flag", 0 )
                .append( "metadata",
                        new BasicDBObject( "c", 1 ).append( "d", 2 ) );
        checkFindResults( file, exp );
    }

    @Test
    public void test5() throws IOException {
        // 使用文件名查询多个
        GridFS gridFS = new GridFS( db, bucketName1 );
        int index = new Random().nextInt( bytesList.size() );
        String fileName = fileNameBase + index;
        List< GridFSDBFile > fileList = gridFS.find( fileName );
        // 检查
        for ( GridFSDBFile file : fileList ) {
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length", bytesList.get( index ).length )
                    .append( "md5", TestTools.getMD5( bytesList.get( index ) ) )
                    .append( "filename", fileName )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null ).append( "_id", file.getId() )
                    .append( "a", file.get( "a" ) )
                    .append( "b", String.valueOf( file.get( "b" ) ) )
                    .append( "flag", index ).append( "metadata",
                            new BasicDBObject( "c",
                                    ( int ) file.get( "a" ) + 1 ).append( "d",
                                            ( int ) file.get( "a" ) + 2 ) );
            checkFindResults( file, exp );
        }
    }

    @Test
    public void test6() throws IOException {
        // 使用文件名查询多个,不带排序
        GridFS gridFS = new GridFS( db, bucketName1 );
        int index = new Random().nextInt( bytesList.size() );
        String fileName = fileNameBase + index;
        List< GridFSDBFile > fileList = gridFS.find( fileName );
        Assert.assertEquals( fileList.size(), fileNum / bytesList.size() );
        // 检查
        for ( GridFSDBFile file : fileList ) {
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length", bytesList.get( index ).length )
                    .append( "md5", TestTools.getMD5( bytesList.get( index ) ) )
                    .append( "filename", fileName )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null ).append( "_id", file.getId() )
                    .append( "a", file.get( "a" ) )
                    .append( "b", String.valueOf( file.get( "b" ) ) )
                    .append( "flag", index ).append( "metadata",
                            new BasicDBObject( "c",
                                    ( int ) file.get( "a" ) + 1 ).append( "d",
                                            ( int ) file.get( "a" ) + 2 ) );
            checkFindResults( file, exp );
        }
    }

    @Test
    public void test7() throws IOException {
        // 使用文件名查询多个,带排序
        GridFS gridFS = new GridFS( db, bucketName1 );
        int index = new Random().nextInt( bytesList.size() );
        String fileName = fileNameBase + index;
        DBObject sort = new BasicDBObject( "a", 1 ).append( "b", -1 );
        List< GridFSDBFile > fileList = gridFS.find( fileName, sort );
        Assert.assertEquals( fileList.size(), fileNum / bytesList.size() );
        // 检查
        for ( GridFSDBFile file : fileList ) {
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length", bytesList.get( index ).length )
                    .append( "md5", TestTools.getMD5( bytesList.get( index ) ) )
                    .append( "filename", fileName )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null ).append( "_id", file.getId() )
                    .append( "a", file.get( "a" ) )
                    .append( "b", String.valueOf( file.get( "b" ) ) )
                    .append( "flag", index ).append( "metadata",
                            new BasicDBObject( "c",
                                    ( int ) file.get( "a" ) + 1 ).append( "d",
                                            ( int ) file.get( "a" ) + 2 ) );
            checkFindResults( file, exp );
        }
    }

    @Test
    public void test8() throws IOException {
        // 使用查询条件查询多个，不带排序
        GridFS gridFS = new GridFS( db, bucketName1 );
        DBObject query = QueryBuilder.start( "a" )
                .greaterThanEquals( fileNum / 2 ).lessThan( fileNum ).get();
        List< GridFSDBFile > fileList = gridFS.find( query );
        Assert.assertEquals( fileList.size(), fileNum - fileNum / 2 );
        // 检查
        for ( GridFSDBFile file : fileList ) {
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length",
                            bytesList.get( ( int ) file.get( "flag" ) ).length )
                    .append( "md5",
                            TestTools.getMD5( bytesList
                                    .get( ( int ) file.get( "flag" ) ) ) )
                    .append( "filename",
                            fileNameBase + ( int ) file.get( "flag" ) )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null ).append( "_id", file.getId() )
                    .append( "a", file.get( "a" ) )
                    .append( "b", String.valueOf( file.get( "b" ) ) )
                    .append( "flag", file.get( "flag" ) ).append( "metadata",
                            new BasicDBObject( "c",
                                    ( int ) file.get( "a" ) + 1 ).append( "d",
                                            ( int ) file.get( "a" ) + 2 ) );
            checkFindResults( file, exp );
        }
    }

    @Test
    public void test9() throws IOException {
        // 使用查询条件查询多个，带排序
        GridFS gridFS = new GridFS( db, bucketName1 );
        DBObject query = QueryBuilder.start( "a" )
                .greaterThanEquals( fileNum / 2 ).lessThan( fileNum ).get();
        DBObject sort = new BasicDBObject( "a", -1 ).append( "b", 1 );
        List< GridFSDBFile > fileList = gridFS.find( query, sort );
        Assert.assertEquals( fileList.size(), fileNum - fileNum / 2 );
        // 检查
        for ( int i = fileNum - 1; i >= fileNum / 2; i-- ) {
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length",
                            bytesList.get( i % bytesList.size() ).length )
                    .append( "md5",
                            TestTools.getMD5(
                                    bytesList.get( i % bytesList.size() ) ) )
                    .append( "filename", fileNameBase + i % bytesList.size() )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null )
                    .append( "_id", fileList.get( fileNum - 1 - i ).getId() )
                    .append( "a", fileList.get( fileNum - 1 - i ).get( "a" ) )
                    .append( "b",
                            String.valueOf( fileList.get( fileNum - 1 - i )
                                    .get( "b" ) ) )
                    .append( "flag",
                            fileList.get( fileNum - 1 - i ).get( "flag" ) )
                    .append( "metadata", new BasicDBObject( "c", i + 1 )
                            .append( "d", i + 2 ) );
            checkFindResults( fileList.get( fileNum - 1 - i ), exp );
        }
    }

    @Test
    public void test10() throws IOException {
        // 集合存在0个文件，使用不存在的文件id或文件名查询文件
        GridFS gridFS = new GridFS( db, bucketName2 );
        String fileName = fileNameBase + "-test10";
        GridFSInputFile gridFSInputFile = gridFS
                .createFile( bytesList.get( 0 ) );
        gridFSInputFile.setFilename( fileName );
        gridFSInputFile.save();
        String fileId = gridFSInputFile.getId().toString();
        gridFS.remove( new ObjectId( fileId ) );
        Assert.assertEquals( gridFS.getFileList().size(), 0 );
        // 使用不存在的文件id进行查询
        GridFSDBFile file1 = gridFS.find( new ObjectId( fileId ) );
        Assert.assertNull( file1 );
        // 使用不存在文件名进行查询
        List< GridFSDBFile > file2 = gridFS.find( fileName );
        Assert.assertEquals( file2.size(), 0 );
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
            Assert.assertEquals( act.get( key ), exp.get( key ) );
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = { bucketName1 + ".files", bucketName1 + ".chunks",
                bucketName2 + ".files", bucketName2 + ".chunks" };
        dropCLByTestResult( context, this.toString(), db, clNames );
        TestTools.LocalFile.removeFile( localPath );
    }
}
