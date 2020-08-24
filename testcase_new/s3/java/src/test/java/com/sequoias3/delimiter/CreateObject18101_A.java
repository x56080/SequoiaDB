package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.Date;
import java.util.List;

/**
 * @Description: 开启版本控制，增加同名对象 testlink-case: seqDB-18101
 *
 * @author wangkexin
 * @Date 2019.04.15
 * @version 1.00
 */
public class CreateObject18101_A extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18101a";
    private String keyName = "/aa/!aab/!atest.png";
    private String expCommPerfix = "/aa/!a";
    private String delimiter = "/!a";
    private int oldFileSize = 1024 * 2;
    private int newFileSize = 1024 * 3;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath1 = localPath + File.separator + "localFile_" + oldFileSize
                + ".txt";
        filePath2 = localPath + File.separator + "localFile_" + newFileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, oldFileSize );
        TestTools.LocalFile.createFile( filePath2, newFileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    private void testCreateObject() throws Exception {
        // 将分隔符设置为/!a （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        // 重复上传同名对象，检查对象LastModified时间在 【第一次上传时间，第二次上传完成后时间】范围内
        s3Client.putObject( bucketName, keyName, new File( filePath1 ) );
        S3Object obj = s3Client.getObject( bucketName, keyName );
        Date dataLowBound = obj.getObjectMetadata().getLastModified();

        s3Client.putObject( bucketName, keyName, new File( filePath2 ) );
        Date dataUpBound = new Date();
        checkResult( filePath2, dataLowBound, dataUpBound, "1" );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkResult( String filePath, Date expDateLowBound,
            Date expDateUpBound, String versionId ) throws Exception {
        S3Object obj = s3Client.getObject(
                new GetObjectRequest( bucketName, keyName, versionId ) );
        ObjectMetadata metadata = obj.getObjectMetadata();
        Date actCreateDate = metadata.getLastModified();
        if ( actCreateDate.before( expDateLowBound )
                || actCreateDate.after( expDateUpBound ) ) {
            Assert.fail( "create time is different! the actCreateDate is : "
                    + actCreateDate.toString() + ",the expDate is in :[ "
                    + expDateLowBound.toString() + " ~ "
                    + expDateUpBound.toString() + " ]" );
        }

        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
        Assert.assertEquals( obj.getKey(), keyName );

        // 通过携带delimiter查询对象列表的对外映射场景检测目录表是否生成新目录，对象元数据表和目录表中数据通过连接db手工校验
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< String > commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), expCommPerfix );
        Assert.assertEquals( result.getObjectSummaries().size(), 0 );
    }
}
