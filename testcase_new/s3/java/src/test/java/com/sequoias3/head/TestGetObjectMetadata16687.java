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
 * test content: 带versionId查询对象，指定range范围 testlink-case: seqDB-16687
 *
 * @author wangkexin
 * @Date 2018.12.10
 * @version 1.00
 */

public class TestGetObjectMetadata16687 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket16687";
    private String userName = "user16687";
    private String roleName = "normal";
    private String keyName = "key16687";
    private int fileSize = 1024 * 100;
    private File localPath = null;
    private String filePath = null;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
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
        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                new File( filePath ) );
        String expEtag = result.getETag();
        String versionid = result.getVersionId();

        HttpHead request = new HttpHead( S3TestBase.s3ClientUrl + "/"
                + bucketName + "/" + keyName + "?versionId=" + versionid );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );

        // 指定range为中间范围1k-99k
        int leftBoundary = 1 * 1024;
        int rightBoundary = 99 * 1024;
        request.setHeader( "Range", "bytes=" + String.valueOf( leftBoundary )
                + "-" + String.valueOf( rightBoundary ) );
        client = RestClient.createHttpClient();
        CloseableHttpResponse resp = RestClient.sendRequest( client, request );

        Assert.assertEquals( resp.getFirstHeader( "ETag" ).getValue(),
                "\"" + expEtag + "\"" );
        Assert.assertEquals(
                resp.getFirstHeader( "x-amz-version-id" ).getValue(),
                versionid );
        Assert.assertEquals( resp.getFirstHeader( "Content-Range" ).getValue(),
                "bytes " + leftBoundary + "-" + rightBoundary + "/"
                        + fileSize );
        Assert.assertEquals( resp.getFirstHeader( "Content-Length" ).getValue(),
                String.valueOf( rightBoundary - leftBoundary + 1 ) );
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
