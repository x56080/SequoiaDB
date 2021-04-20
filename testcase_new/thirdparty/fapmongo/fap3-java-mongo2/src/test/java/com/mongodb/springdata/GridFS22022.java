package com.mongodb.springdata;

import java.io.ByteArrayInputStream;
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
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.gridfs.GridFsCriteria;
import org.springframework.data.mongodb.gridfs.GridFsTemplate;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.gridfs.GridFSDBFile;
import com.mongodb.gridfs.GridFSFile;
import com.mongodb.utils.MongodbTestBase;
import com.mongodb.utils.TestTools;

/**
 * @Description: seqDB-22022:查询文件
 * @author fanyu
 * @Date 2020/4/8
 * @version 1.00
 */
public class GridFS22022 extends MongodbTestBase {
    private GridFsTemplate gridFsTemplate1;
    private GridFsTemplate gridFsTemplate2;
    private String fileNameBase = "fs22022-";
    private String bucketName1;
    private String bucketName2;
    private int fileNum = 20;
    private File localPath;
    private List< byte[] > bytesList = new ArrayList<>();
    private List< String > md5List = new ArrayList<>();
    private List< String > fileIdList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws UnknownHostException {
        localPath = new File( File.separator + "tmp" + File.separator
                + File.separator + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        Random random = new Random();
        for ( int i = 0; i < Math.min( 5, fileNum ); i++ ) {
            byte[] bytes = new byte[ 1024 * 200 + i ];
            random.nextBytes( bytes );
            bytesList.add( bytes );
            md5List.add( TestTools.getMD5( bytes ) );
        }

        bucketName1 = springDBNameWithVersion + "_bucket22022A";
        bucketName2 = springDBNameWithVersion + "_bucket22022B";

        // 准备文件
        // 自定义桶1
        gridFsTemplate1 = new GridFsTemplate( mongoDbFactory, converter,
                bucketName1 );
        // 自定义桶2
        gridFsTemplate2 = new GridFsTemplate( mongoDbFactory, converter,
                bucketName2 );
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSFile gridFSFile = gridFsTemplate1.store(
                    new ByteArrayInputStream(
                            bytesList.get( i % bytesList.size() ) ),
                    fileNameBase + i % bytesList.size() );
            gridFSFile.put( "flag", i % bytesList.size() );
            gridFSFile.put( "a", i );
            gridFSFile.put( "b", String.valueOf( i ) );
            gridFSFile.setMetaData(
                    new BasicDBObject( "c", i + 1 ).append( "d", i + 2 ) );
            gridFSFile.save();
            fileIdList.add( gridFSFile.getId().toString() );
        }
    }

    @Test
    public void test1() throws IOException {
        // 使用文件id查询单个文件
        Query query = new Query( GridFsCriteria.where( "_id" )
                .is( new ObjectId( fileIdList.get( 0 ) ) ) );
        GridFSDBFile file = gridFsTemplate1.findOne( query );
        // 检查
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytesList.get( 0 ).length )
                .append( "md5", md5List.get( 0 ) )
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
        int index = new Random().nextInt( bytesList.size() );
        String fileName = fileNameBase + index;
        Query query = new Query(
                GridFsCriteria.whereFilename().is( fileName ) );
        GridFSDBFile file = gridFsTemplate1.findOne( query );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytesList.get( index ).length )
                .append( "md5", md5List.get( index ) )
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
        Query query = new Query(
                GridFsCriteria.where( "a" ).gte( fileNum / 2 ).lt( fileNum ) );
        GridFSDBFile file = gridFsTemplate1.findOne( query );
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length",
                        bytesList.get( ( int ) file.get( "flag" ) ).length )
                .append( "md5", md5List.get( ( int ) file.get( "flag" ) ) )
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
        Query query = new Query( GridFsCriteria.where( "_id" )
                .is( new ObjectId( fileIdList.get( 0 ) ) ) );
        List< GridFSDBFile > fileList = gridFsTemplate1.find( query );
        Assert.assertEquals( fileList.size(), 1, fileList.toString() );
        GridFSDBFile file = fileList.get( 0 );
        // 检查
        BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                .append( "length", bytesList.get( 0 ).length )
                .append( "md5", md5List.get( 0 ) )
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
        // 使用文件名查询多个,不带排序
        int index = new Random().nextInt( bytesList.size() );
        String fileName = fileNameBase + index;
        Query query = new Query(
                GridFsCriteria.whereFilename().is( fileName ) );
        List< GridFSDBFile > fileList = gridFsTemplate1.find( query );
        Assert.assertEquals( fileList.size(), fileNum / bytesList.size(),
                fileList.toString() );
        // 检查
        for ( int i = 0; i < fileList.size(); i++ ) {
            int num = bytesList.size() * i + index;
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length", bytesList.get( index ).length )
                    .append( "md5", md5List.get( index ) )
                    .append( "filename", fileName )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null )
                    .append( "_id", new ObjectId( fileIdList.get( num ) ) )
                    .append( "a", num ).append( "b", String.valueOf( num ) )
                    .append( "flag", index )
                    .append( "metadata", new BasicDBObject( "c", num + 1 )
                            .append( "d", num + 2 ) );
            checkFindResults( fileList.get( i ), exp );
        }
    }

    @Test
    public void test7() throws IOException {
        // 使用文件名查询多个,带排序
        int index = new Random().nextInt( bytesList.size() );
        String fileName = fileNameBase + index;
        Query query = new Query(
                GridFsCriteria.whereFilename().is( fileName ) );
        query.with( new Sort( Sort.Direction.ASC, "a" ) );
        List< GridFSDBFile > fileList = gridFsTemplate1.find( query );
        Assert.assertEquals( fileList.size(), fileNum / bytesList.size(),
                fileList.toString() );
        // 检查
        for ( int i = 0; i < fileList.size(); i++ ) {
            int num = bytesList.size() * i + index;
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length", bytesList.get( index ).length )
                    .append( "md5", md5List.get( index ) )
                    .append( "filename", fileName )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null )
                    .append( "_id", new ObjectId( fileIdList.get( num ) ) )
                    .append( "a", num ).append( "b", String.valueOf( num ) )
                    .append( "flag", index )
                    .append( "metadata", new BasicDBObject( "c", num + 1 )
                            .append( "d", num + 2 ) );
            checkFindResults( fileList.get( i ), exp );
        }
    }

    @Test
    public void test8() throws IOException {
        // 使用查询条件查询多个，不带排序
        Query query = new Query(
                GridFsCriteria.where( "a" ).gte( fileNum / 2 ).lt( fileNum ) );
        List< GridFSDBFile > fileList = gridFsTemplate1.find( query );
        Assert.assertEquals( fileList.size(), fileNum - fileNum / 2 );
        // 检查
        for ( int i = 0; i < fileNum - fileNum / 2; i++ ) {
            int num = i + fileNum / 2;
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length",
                            bytesList.get( num % bytesList.size() ).length )
                    .append( "md5", md5List.get( num % bytesList.size() ) )
                    .append( "filename", fileNameBase + num % bytesList.size() )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null )
                    .append( "_id", new ObjectId( fileIdList.get( num ) ) )
                    .append( "a", num ).append( "b", String.valueOf( num ) )
                    .append( "flag", num % bytesList.size() )
                    .append( "metadata", new BasicDBObject( "c", num + 1 )
                            .append( "d", num + 2 ) );
            checkFindResults( fileList.get( i ), exp );
        }
    }

    @Test
    public void test9() throws IOException {
        // 使用查询条件查询多个，带排序
        Query query = new Query(
                GridFsCriteria.where( "a" ).gte( fileNum / 2 ).lt( fileNum ) );
        query.with( new Sort( Sort.Direction.DESC, "a", "b" ) );
        List< GridFSDBFile > fileList = gridFsTemplate1.find( query );
        Assert.assertEquals( fileList.size(), fileNum - fileNum / 2 );
        // 检查
        for ( int i = 0; i < fileNum - fileNum / 2; i++ ) {
            int num = fileNum - 1 - i;
            BasicDBObject exp = new BasicDBObject( "chunkSize", 261120 )
                    .append( "length",
                            bytesList.get( num % bytesList.size() ).length )
                    .append( "md5", md5List.get( num % bytesList.size() ) )
                    .append( "filename", fileNameBase + num % bytesList.size() )
                    .append( "contentType", null ).append( "aliases", null )
                    .append( "uploadDate", null )
                    .append( "_id", new ObjectId( fileIdList.get( num ) ) )
                    .append( "a", num ).append( "b", String.valueOf( num ) )
                    .append( "flag", num % bytesList.size() )
                    .append( "metadata", new BasicDBObject( "c", num + 1 )
                            .append( "d", num + 2 ) );
            checkFindResults( fileList.get( i ), exp );
        }
    }

    @Test
    public void test10() throws IOException {
        // 集合存在0个文件，使用不存在的文件id或文件名查询文件
        String fileName = fileNameBase + 0;
        gridFsTemplate2.store( new ByteArrayInputStream( bytesList.get( 0 ) ),
                fileName );
        Query query = new Query(
                GridFsCriteria.whereFilename().is( fileName ) );
        gridFsTemplate2.delete( query );
        Assert.assertEquals( gridFsTemplate2.find( query ).size(), 0 );
        // 使用不存在文件名进行查询
        List< GridFSDBFile > file2 = gridFsTemplate2.find( query );
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
            Assert.assertEquals( act.get( key ), exp.get( key ),
                    "actKey = " + key + ",actValue = " + act.get( key ) );
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = { bucketName1 + ".files", bucketName1 + ".chunks",
                bucketName2 + ".files", bucketName2 + ".chunks" };
        dropCLByTestResult( context, this.toString(), mongoTemplate, clNames );
        TestTools.LocalFile.removeFile( localPath );
    }
}
