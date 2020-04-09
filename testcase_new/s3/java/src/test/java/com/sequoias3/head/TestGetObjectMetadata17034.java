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
import org.apache.http.client.methods.HttpHead;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * test content: 带versionId查询对象，指定ifModifiedSince与对象LastModified时间相等
 * testlink-case: seqDB-17034
 *
 * @author wangkexin
 * @Date 2019.01.02
 * @version 1.00
 */

public class TestGetObjectMetadata17034 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket17034";
    private String userName = "user17034";
    private String roleName = "normal";
    private String keyName = "key17034";
    private String content = "content17034";
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
        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                content );
        String versionId = result.getVersionId();

        ObjectMetadata metadata = s3Client
                .getObjectMetadata( new GetObjectMetadataRequest( bucketName,
                        keyName, versionId ) );
        Date modifiedSince = metadata.getLastModified();

        HttpHead request = new HttpHead( S3TestBase.s3ClientUrl + "/"
                + bucketName + "/" + keyName + "?versionId=" + versionId );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        request.setHeader( "If-Modified-Since",
                HeadUtils.getGMTDate( modifiedSince ) );
        client = RestClient.createHttpClient();
        try {
            RestClient.sendRequest( client, request );
            Assert.fail(
                    "getting objects should fail when specifying 'If-Modified-Since' equals 'LastModified'" );
        } catch ( Exception e ) {
            Assert.assertNotEquals( e.getMessage().indexOf( "errcode=304" ),
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
