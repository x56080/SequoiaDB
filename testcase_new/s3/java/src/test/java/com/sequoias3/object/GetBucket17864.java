package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: GET /s3/bucketname验证 testlink-case: seqDB-17864
 *
 * @author wangkexin
 * @Date 2019.02.15
 * @version 1.00
 */
public class GetBucket17864 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String userName = "user17864";
    private String bucketName = "bucket17864";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;
    private String[] accessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testCreateBucket() throws Exception {
        s3Client.createBucket( bucketName );
        try {
            getBucket( bucketName );
        } catch ( Exception e ) {
            Assert.assertTrue( e.getMessage().contains(
                    "Please use list objects V2 request instead of list objects V1." ),
                    e.getMessage() );
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

    private void getBucket( String bucketName ) throws Exception {
        HttpGet request = new HttpGet(
                S3TestBase.s3ClientUrl + "/" + bucketName );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        client = RestClient.createHttpClient();
        RestClient.sendRequest( client, request );
    }

}
