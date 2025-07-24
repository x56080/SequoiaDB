package com.mongodb.springdata;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.types.ObjectId;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.gridfs.GridFsCriteria;
import org.springframework.data.mongodb.gridfs.GridFsTemplate;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.gridfs.GridFSFile;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-22023:删除文件
 * @author fanyu
 * @Date 2020/4/9
 * @version 1.00
 */
public class GridFS22023 extends MongodbTestBase {
    private GridFsTemplate gridFsTemplate1;
    private GridFsTemplate gridFsTemplate2;
    private String fileNameBase = "fs22023";
    private String bucketName1;
    private String bucketName2;
    private byte[] bytes = new byte[ 1024 ];

    @BeforeClass
    public void setUp() throws UnknownHostException {
        new Random().nextBytes( bytes );
        bucketName1 = springDBNameWithVersion + "_bucket22023A";
        bucketName2 = springDBNameWithVersion + "_bucket22023B";
        // 自定义桶1
        gridFsTemplate1 = new GridFsTemplate( mongoDbFactory, converter,
                bucketName1 );
        // 自定义桶2
        gridFsTemplate2 = new GridFsTemplate( mongoDbFactory, converter,
                bucketName2 );
    }

    @Test
    public void test1() throws IOException {
        // 集合存在同名文件，使用查询条件删除文件
        // 创建文件
        String fileName = fileNameBase + "test1";
        int fileNum = 10;
        List< String > fileIdList = new ArrayList<>();
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSFile gridFSFile = gridFsTemplate1
                    .store( new ByteArrayInputStream( bytes ), fileName );
            fileIdList.add( gridFSFile.getId().toString() );
        }
        // 使用文件id删除文件
        Query query = new Query( GridFsCriteria.where( "_id" )
                .is( new ObjectId( fileIdList.get( 0 ) ) ) );
        Assert.assertEquals( gridFsTemplate1.find( query ).size(), 1 );
        gridFsTemplate1.delete( query );
        Assert.assertNull( gridFsTemplate1.findOne( query ) );
        Assert.assertEquals( gridFsTemplate1.find( new Query() ).size(),
                fileNum - 1 );

        // 使用不存在的文件id删除文件
        gridFsTemplate1.delete( query );
        Assert.assertEquals( gridFsTemplate1.find( new Query() ).size(),
                fileNum - 1 );

        // 使用不存在的文件名删除文件
        query = new Query(
                GridFsCriteria.whereFilename().is( fileName + "-notexist" ) );
        Assert.assertEquals( gridFsTemplate1.find( query ).size(), 0 );
        gridFsTemplate1.delete( query );
        Assert.assertEquals( gridFsTemplate1.find( new Query() ).size(),
                fileNum - 1 );

        // 使用文件名删除文件
        query = new Query( GridFsCriteria.whereFilename().is( fileName ) );
        Assert.assertEquals( gridFsTemplate1.find( query ).size(),
                fileNum - 1 );
        gridFsTemplate1.delete( query );
        Assert.assertEquals( gridFsTemplate1.find( query ).size(), 0 );

        // 集合不存在文件删除文件
        gridFsTemplate1.delete( query );
        Assert.assertEquals( gridFsTemplate1.find( query ).size(), 0 );
    }

    @Test
    public void test2() throws IOException {
        // 集合存在多个不同名文件，使用查询条件删除文件
        // 创建文件
        int fileNum = 10;
        List< String > fileIdList = new ArrayList<>();
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSFile gridFSFile = gridFsTemplate2.store(
                    new ByteArrayInputStream( bytes ), fileNameBase + i );
            gridFSFile.put( "a", i );
            gridFSFile.save();
            fileIdList.add( gridFSFile.getId().toString() );
        }
        // 使用查询条件删除文件
        Query query = new Query(
                GridFsCriteria.where( "a" ).gte( 0 ).lt( fileNum / 2 ) );
        Assert.assertEquals( gridFsTemplate2.find( query ).size(),
                fileNum / 2 );
        gridFsTemplate2.delete( query );
        Assert.assertEquals( gridFsTemplate2.find( query ).size(), 0 );

        // 指定不存在的字段当查询条件删除文件
        query = new Query(
                GridFsCriteria.where( "b" ).gte( 0 ).lt( fileNum / 2 ) );
        Assert.assertEquals( gridFsTemplate2.find( query ).size(), 0 );
        gridFsTemplate2.delete( query );
        Assert.assertEquals( gridFsTemplate2.find( new Query() ).size(),
                fileNum / 2 );

        // 匹配不到文件，删除文件
        query = new Query( GridFsCriteria.where( "a" ).gte( fileNum ) );
        Assert.assertEquals( gridFsTemplate2.find( query ).size(), 0 );
        gridFsTemplate2.delete( query );
        Assert.assertEquals( gridFsTemplate2.find( new Query() ).size(),
                fileNum / 2 );

        // 删除所有文件
        gridFsTemplate2.delete( new Query() );
        Assert.assertEquals( gridFsTemplate2.find( new Query() ).size(), 0 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = { bucketName1 + ".files", bucketName1 + ".chunks",
                bucketName2 + ".files", bucketName2 + ".chunks" };
        dropCLByTestResult( context, this.toString(), mongoTemplate, clNames );
    }
}
