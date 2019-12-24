package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PutObjectResult;
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
 * @Description seqDB-16696:head object with match conditions: ifMatch and
 *              ifNoneMatch,mismatch ifMatch
 * @author wuyan
 * @Date 2018.12.18
 * @version 1.00
 */

public class HeadObject16696 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket16696";
    private String userName = "user16696";
    private String roleName = "normal";
    private String key = "/test/key16696";
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    private void testHeadObject() throws Exception {

        PutObjectResult resultV1 = s3Client
                .putObject( bucketName, key, "testobject16696v100" );
        String etagV1 = resultV1.getETag();
        String versionidV1 = resultV1.getVersionId();
        PutObjectResult resultV2 = s3Client
                .putObject( bucketName, key, "testobject16696v2" );
        String etagV2 = resultV2.getETag();
        s3Client.putObject( bucketName, key, "testobject16696v3" );

        // mismatch etagV1
        s3Client.deleteVersion( bucketName, key, versionidV1 );
        HttpHead request = new HttpHead(
                S3TestBase.s3ClientUrl + "/" + bucketName + "/" + key );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        request.setHeader( "If-Match", etagV1 );
        request.setHeader( "If-None-Match", etagV2 );
        try {
            client = RestClient.createHttpClient();
            RestClient.sendRequest( client, request );
            Assert.fail( "not match object must be return a 412!" );
        } catch ( Exception e ) {
            // return a 412 ( precondition failed)
            Assert.assertTrue( e.getMessage().contains( "errcode=412" ),
                    "errorMsg is:" + e.getMessage() );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
