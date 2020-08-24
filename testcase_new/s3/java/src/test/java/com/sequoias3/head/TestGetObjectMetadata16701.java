package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.HeadUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * @Description: 指定ifMatch和ifUnModifiedSince条件查询对象 testlink-case: seqDB-16701
 *
 * @author wangkexin
 * @Date 2018.12.11
 * @version 1.00
 */

public class TestGetObjectMetadata16701 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket16701";
    private String userName = "user16701";
    private String roleName = "normal";
    private String keyName = "key16701";
    private String content = "content16701";
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
        s3Client.putObject( bucketName, keyName, content + "v2" );
        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                content + "v3" );
        String etag = result.getETag();
        String versionid = result.getVersionId();

        ObjectMetadata metadata = s3Client
                .getObjectMetadata( new GetObjectMetadataRequest( bucketName,
                        keyName, versionid ) );
        Date actDate = metadata.getLastModified();

        // ifUnModifiedSince指定为时间A，时间A后该对象未修改；ifMatch指定为该对象当前版本的Etag值
        HttpHead request = new HttpHead(
                S3TestBase.s3ClientUrl + "/" + bucketName + "/" + keyName );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        request.setHeader( "If-Unmodified-Since",
                HeadUtils.getModifiedGMTDate( actDate, 1 ) );
        request.setHeader( "If-Match", etag );

        client = RestClient.createHttpClient();
        CloseableHttpResponse resp = RestClient.sendRequest( client, request );
        Assert.assertEquals( resp.getFirstHeader( "ETag" ).getValue(),
                "\"" + etag + "\"" );
        Assert.assertEquals(
                resp.getFirstHeader( "x-amz-version-id" ).getValue(),
                versionid );
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
