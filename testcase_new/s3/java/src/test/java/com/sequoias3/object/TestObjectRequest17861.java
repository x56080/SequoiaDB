package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.DeleteObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * test content: Put/Get/Delete/HeadObject请求携带特殊字符校验 testlink-case: seqDB-17861
 *
 * @author wangkexin
 * @Date 2019.02.15
 * @version 1.00
 */
public class TestObjectRequest17861 extends S3TestBase {
    private String bucketName = "bucket17861";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testasciiKeyName() throws Exception {
        // 测试特殊字符，如包含有 '\n' ,'\r','\u0085'的字符(对应十进制数字10，13，133)
        String asciiKeyName = new String();
        for ( int i = 1; i < 32; i++ ) {
            asciiKeyName += ( char ) i;
        }
        for ( int i = 127; i < 256; i++ ) {
            asciiKeyName += ( char ) i;
        }

        // test Put Object Request
        PutObjectRequest putObjectRequest = new PutObjectRequest( bucketName,
                asciiKeyName, new File( filePath ) );
        ObjectMetadata metadata = new ObjectMetadata();
        Map< String, String > xMeta = new HashMap< String, String >();
        xMeta.put( "meta17861", "17861" );
        metadata.setUserMetadata( xMeta );
        putObjectRequest.withMetadata( metadata );
        PutObjectResult result = s3Client.putObject( putObjectRequest );
        String actMd5 = result.getETag();
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( actMd5, expMd5,
                "putobjectrequest : md5 is wrong! the key name is : "
                        + asciiKeyName );

        // test Get Object Request
        GetObjectRequest getObjectRequest = new GetObjectRequest( bucketName,
                asciiKeyName );
        S3Object object = s3Client.getObject( getObjectRequest );
        actMd5 = object.getObjectMetadata().getETag();
        Assert.assertEquals( actMd5, expMd5,
                "getobjectrequest : md5 is wrong! the key name is : "
                        + asciiKeyName );

        // test Head Object Request
        Assert.assertTrue(
                s3Client.doesObjectExist( bucketName, asciiKeyName ) );
        GetObjectMetadataRequest getObjectMetadataRequest = new GetObjectMetadataRequest(
                bucketName, asciiKeyName );
        ObjectMetadata metadata2 = s3Client
                .getObjectMetadata( getObjectMetadataRequest );
        actMd5 = metadata2.getETag();
        Assert.assertEquals( actMd5, expMd5,
                "getobjectrequest : md5 is wrong! the key name is : "
                        + asciiKeyName );

        // test Delete Object Request
        DeleteObjectRequest deleteObjectRequest = new DeleteObjectRequest(
                bucketName, asciiKeyName );
        s3Client.deleteObject( deleteObjectRequest );
        Assert.assertFalse(
                s3Client.doesObjectExist( bucketName, asciiKeyName ) );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
