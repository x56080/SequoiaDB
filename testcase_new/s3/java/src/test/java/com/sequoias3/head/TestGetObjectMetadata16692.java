package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: 指定ifNoneMatch条件，带versionId查询对象 testlink-case: seqDB-16692
 *
 * @author wangkexin
 * @Date 2018.12.10
 * @version 1.00
 */

public class TestGetObjectMetadata16692 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket16692";
    private String userName = "user16692";
    private String roleName = "normal";
    private String keyName = "key16692";
    private String content = "content16692";
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testGetObjectMetadata() throws Exception {
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, content + "v1" );
        PutObjectResult resultV2 = s3Client.putObject( bucketName, keyName,
                content + "v2" );
        PutObjectResult resultV3 = s3Client.putObject( bucketName, keyName,
                content + "v3" );
        String etagV2 = resultV2.getETag();
        String etagV3 = resultV3.getETag();
        String versionidV3 = resultV3.getVersionId();

        HttpHead request = new HttpHead( S3TestBase.s3ClientUrl + "/"
                + bucketName + "/" + keyName + "?versionId=" + versionidV3 );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );

        // 设置Etag值为当前版本对象的Etag值（v3版本）
        request.setHeader( "If-None-Match", etagV3 );
        client = RestClient.createHttpClient();
        try {
            RestClient.sendRequest( client, request );
            Assert.fail( "exp fail but found success" );
        } catch ( Exception e ) {
            Assert.assertNotEquals( e.getMessage().indexOf( "errcode=304" ),
                    -1 );
        }

        request = new HttpHead( S3TestBase.s3ClientUrl + "/" + bucketName + "/"
                + keyName + "?versionId=" + versionidV3 );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        // 设置Etag值不为当前版本对象的Etag值（v2版本）
        request.setHeader( "If-None-Match", etagV2 );
        client = RestClient.createHttpClient();
        CloseableHttpResponse resp = RestClient.sendRequest( client, request );

        Assert.assertEquals( resp.getFirstHeader( "ETag" ).getValue(),
                "\"" + etagV3 + "\"" );
        Assert.assertEquals(
                resp.getFirstHeader( "x-amz-version-id" ).getValue(),
                versionidV3 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
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
