package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.util.Md5Utils;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 * test content: 开启版本控制，增加对象，指定headers请求参数 testlink-case: seqDB-16346
 *
 * @author wangkexin
 * @Date 2018.11.12
 * @version 1.00
 */
public class CreateObject16346 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private String bucketName = "bucket16346";
    private String keyName = "object16346";
    private AmazonS3 s3Client = null;
    private File localPath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        s3Client = CommLib.buildS3Client();

        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testCreateBucket() throws Exception {
        Date currdate = new Date();
        String content_encoding = "gzip16346";
        String content_type = "plain/text16346";
        String content = "testContent16346";
        String date = "Thu, 29 Nov 2018 07:49:16 GMT";
        String x_amz_date = "Thu, 29 Nov 2018 16:47:02 GMT";
        String x_amz_meta_x = "myparameter16346";
        String cache_control = "no-cache";
        String content_disposition = "testDisposition16346";
        String expires = getGMTDate( currdate );

        HttpPut request = new HttpPut(
                S3TestBase.s3ClientUrl + "/" + bucketName + "/" + keyName );
        // RequestHeaders:
        request.setHeader( "Authorization",
                "Credential=ABCDEFGHIJKLMNOPQRST" + "/" );
        request.setHeader( "Content-Encoding", content_encoding );
        request.setHeader( "Content-Type", content_type );
        request.setHeader( "Content-MD5",
                Md5Utils.md5AsBase64( content.getBytes() ) );
        request.setHeader( "Date", date );
        request.setHeader( "x-amz-date", x_amz_date );
        request.setHeader( "x-amz-meta-myparameter", x_amz_meta_x );
        request.setHeader( "Cache-Control", cache_control );
        request.setHeader( "Content-Disposition", content_disposition );
        request.setHeader( "Expires", expires );

        // Requeatbody:
        StringEntity testString = new StringEntity( content,
                StandardCharsets.UTF_8 );
        request.setEntity( testString );
        client = RestClient.createHttpClient();
        RestClient.sendRequest( client, request );

        S3Object object = s3Client.getObject( bucketName, keyName );
        Assert.assertEquals( object.getObjectMetadata().getContentLength(),
                content.length(), "ContentLength is wrong" );
        Assert.assertEquals( object.getObjectMetadata().getContentEncoding(),
                content_encoding, "ContentEncoding is wrong" );
        Assert.assertEquals( object.getObjectMetadata().getContentType(),
                content_type, "ContentType is wrong" );
        Assert.assertEquals( object.getObjectMetadata().getCacheControl(),
                cache_control, "CacheControl is wrong" );
        Assert.assertEquals( object.getObjectMetadata().getContentDisposition(),
                content_disposition, "ContentDisposition is wrong" );
        Assert.assertEquals(
                getGMTDate( object.getObjectMetadata().getHttpExpiresDate() ),
                expires, "Expires is wrong" );
        Assert.assertEquals( object.getObjectMetadata().getUserMetadata()
                .get( "myparameter" ), x_amz_meta_x, "x-amz-meta-* is wrong" );
        String actMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( actMd5, TestTools.getMD5( content.getBytes() ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
    }

    private String getGMTDate( Date date ) {
        SimpleDateFormat sdf = new SimpleDateFormat(
                "EEE, dd MMM yyyy HH:mm:ss z", Locale.US );
        sdf.setTimeZone( TimeZone.getTimeZone( "GMT" ) );
        String rfc1123 = sdf.format( date );
        return rfc1123;
    }
}