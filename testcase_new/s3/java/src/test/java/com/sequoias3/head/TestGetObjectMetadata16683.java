package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 带versionId查询对象，不匹配ifMatch条件 testlink-case: seqDB-16683
 *
 * @author wangkexin
 * @Date 2018.12.10
 * @version 1.00
 */

public class TestGetObjectMetadata16683 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket16683";
    private String userName = "user16683";
    private String roleName = "normal";
    private String keyName = "key16683";
    private String content = "content16683";
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
        PutObjectResult resultV1 = s3Client.putObject( bucketName, keyName,
                content + "v1" );
        String versionIdV1 = resultV1.getVersionId();

        PutObjectResult resultV2 = s3Client.putObject( bucketName, keyName,
                content + "v2" );
        String etagV2 = resultV2.getETag();

        // 设置Etag值和指定versionID对应值不一致
        HttpHead request = new HttpHead( S3TestBase.s3ClientUrl + "/"
                + bucketName + "/" + keyName + "?versionId=" + versionIdV1 );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        request.setHeader( "If-Match", etagV2 );
        client = RestClient.createHttpClient();
        try {
            RestClient.sendRequest( client, request );
            Assert.fail(
                    "get object with incorrect etag value should be fail!" );
        } catch ( Exception e ) {
            Assert.assertNotEquals( e.getMessage().indexOf( "errcode=412" ),
                    -1 );
        }
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
