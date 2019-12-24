package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * test content: 查询对象，指定range范围不匹配 testlink-case: seqDB-16690
 *
 * @author wangkexin
 * @Date 2018.12.10
 * @version 1.00
 */

public class TestGetObjectMetadata16690 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket16690";
    private String userName = "user16690";
    private String roleName = "normal";
    private String keyName = "key16690";
    private int fileSize = 1024 * 100;
    private File localPath = null;
    private String filePath = null;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testGetObjectMetadata() throws Exception {
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        PutObjectResult result = s3Client
                .putObject( bucketName, keyName, new File( filePath ) );
        String versionid = result.getVersionId();
        String etag = result.getETag();

        HttpHead request = new HttpHead(
                S3TestBase.s3ClientUrl + "/" + bucketName + "/" + keyName
                        + "?versionId=" + versionid );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );

        // 指定range超过边界值,右边界
        int leftBoundary = fileSize - 10;
        int rightBoundary = fileSize + 1;

        request.setHeader( "Range",
                "bytes=" + String.valueOf( leftBoundary ) + "-" + String
                        .valueOf( rightBoundary ) );
        client = RestClient.createHttpClient();
        CloseableHttpResponse resp1 = RestClient.sendRequest( client, request );
        Assert.assertEquals( resp1.getFirstHeader( "ETag" ).getValue(),
                "\"" + etag + "\"" );
        Assert.assertEquals(
                resp1.getFirstHeader( "Content-Length" ).getValue(),
                String.valueOf( ( fileSize - 1 ) - leftBoundary + 1 ) );

        // 指定range超过边界值,左边界
        request.setHeader( "Range",
                "bytes=-" + String.valueOf( fileSize + 1 ) );
        client = RestClient.createHttpClient();
        CloseableHttpResponse resp2 = RestClient.sendRequest( client, request );
        Assert.assertEquals( resp2.getFirstHeader( "ETag" ).getValue(),
                "\"" + etag + "\"" );
        Assert.assertEquals(
                resp2.getFirstHeader( "Content-Length" ).getValue(),
                String.valueOf( fileSize ) );

        // 指定range不在范围[0k~99k]内
        leftBoundary = fileSize + 100;
        rightBoundary = fileSize + 101;

        request.setHeader( "Range",
                "bytes=" + String.valueOf( leftBoundary ) + "-" + String
                        .valueOf( rightBoundary ) );
        client = RestClient.createHttpClient();
        try {
            RestClient.sendRequest( client, request );
            Assert.fail( "exp fail but found success" );
        } catch ( Exception e ) {
            Assert.assertNotEquals( e.getMessage().indexOf( "errcode=416" ),
                    -1 );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
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
