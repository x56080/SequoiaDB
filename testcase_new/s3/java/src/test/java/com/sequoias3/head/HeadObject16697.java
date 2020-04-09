package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 * @Description seqDB-16697:head object with match conditions: ifNoneMatch and
 *              ifUnModifiedSince
 * @author wuyan
 * @Date 2018.12.18
 * @version 1.00
 */

public class HeadObject16697 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String key = "/head/key16697";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        ObjectUtils.deleteObjectAllVersions( s3Client,
                S3TestBase.enableVerBucketName, key );
    }

    @Test
    private void testHeadObject() throws Exception {
        PutObjectResult resultV1 = s3Client.putObject(
                S3TestBase.enableVerBucketName, key, "testobject16697v100" );
        String etagV1 = resultV1.getETag();
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                "testobject16697v2" );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                "testobject16697v3" );

        GetObjectMetadataRequest getRequest = new GetObjectMetadataRequest(
                S3TestBase.enableVerBucketName, key );
        ObjectMetadata result = s3Client.getObjectMetadata( getRequest );
        String etagV3 = result.getETag();
        Date date = result.getLastModified();
        String unmodifiedTime = getDateByRfc( date );

        HttpHead request = new HttpHead( S3TestBase.s3ClientUrl + "/"
                + S3TestBase.enableVerBucketName + "/" + key );
        request.setHeader( "Authorization",
                "Credential=" + S3TestBase.s3AccessKeyId + "/" );
        request.setHeader( "If-Unmodified-Since", unmodifiedTime );
        request.setHeader( "If-None-Match", etagV1 );
        client = RestClient.createHttpClient();
        CloseableHttpResponse resp = RestClient.sendRequest( client, request );
        Assert.assertEquals( resp.getFirstHeader( "Etag" ).getValue(),
                "\"" + etagV3 + "\"" );
        Assert.assertEquals(
                resp.getFirstHeader( "x-amz-version-id" ).getValue(), "2" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client,
                        S3TestBase.enableVerBucketName, key );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private String getDateByRfc( Date date ) {
        // time delayed by 1 second to reduce time error
        long time = date.getTime() + 1000;
        SimpleDateFormat sdf = new SimpleDateFormat(
                "EEE, dd MMM yyyy HH:mm:ss z", Locale.US );
        sdf.setTimeZone( TimeZone.getTimeZone( "GMT" ) );
        String dateStr = sdf.format( new Date( time ) );
        return dateStr;
    }
}
