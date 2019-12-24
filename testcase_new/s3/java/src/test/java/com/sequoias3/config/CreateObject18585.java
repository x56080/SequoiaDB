package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.HeadBucketRequest;
import com.amazonaws.services.s3.model.Owner;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.HttpStatusCodeException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: 关闭鉴权，设置鉴权头部错误，上传对象 testlink-case: seqDB-18585
 *
 * @author wangkexin
 * @Date 2019.06.20
 * @version 1.00
 */
@Test(groups = "authorizationoff") public class CreateObject18585
        extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String userName = "user18585";
    private String roleName = "normal";
    private String keyName = "key18585";
    private AmazonS3 s3Client = null;
    private AmazonS3 s3ClientNorMal = null;
    private String[] accessKeys = null;

    @DataProvider(name = "authorizationProvider")
    public Object[][] generateAuthorization() {
        return new Object[][] {
                // test a : authorization 为空
                new Object[] { "bucket18585", "" },
                // test b : authorization 为version2版本
                new Object[] { "bucket18585v2",
                        "AWS " + accessKeys[ 0 ] + ":signature" },
                // test c : authorization 为version4版本
                new Object[] { "bucket18585v4",
                        UserCommDefind.authValPre + accessKeys[ 0 ] + "/" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3ClientNorMal = CommLib
                .buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client = CommLib.buildS3Client();
    }

    @SuppressWarnings("deprecation")
    @Test(dataProvider = "authorizationProvider")
    private void testCreateObject( String bucketName, String authorization )
            throws Exception {
        String tmpContent = "content18585" + authorization;

        // 鉴权关闭，直接使用默认用户，携带鉴权消息被忽略
        // create bucket
        createBucket( bucketName, authorization );

        // check head bucket
        headBucket( bucketName, authorization );
        s3Client.headBucket( new HeadBucketRequest( bucketName ) );

        checkCreateBucketResult( s3ClientNorMal, bucketName,
                S3TestBase.s3UserName );

        putObject( bucketName, keyName, tmpContent, authorization );

        // check head object
        headObject( bucketName, keyName, authorization );

        String actEtg = getObject( bucketName, keyName, authorization );
        Assert.assertEquals( actEtg,
                TestTools.getMD5( tmpContent.getBytes() ) );

        deleteObjet( bucketName, keyName, authorization );
        deleteBucket( bucketName, authorization );
        Assert.assertFalse( s3Client.doesBucketExist( bucketName ) );

        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generateAuthorization().length ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
            if ( s3ClientNorMal != null ) {
                s3ClientNorMal.shutdown();
            }
        }
    }

    private void createBucket( String bucketName, String authorization )
            throws UnsupportedEncodingException {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( URLEncoder.encode( bucketName, "UTF-8" ) )
                    .setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private void headBucket( String bucketName, String authorization )
            throws UnsupportedEncodingException {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( URLEncoder.encode( bucketName, "UTF-8" ) )
                    .setRequestMethod( HttpMethod.HEAD )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazonHead( e );
        }
    }

    private void deleteBucket( String bucketName, String authorization )
            throws UnsupportedEncodingException {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( URLEncoder.encode( bucketName, "UTF-8" ) )
                    .setRequestMethod( HttpMethod.DELETE )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private void checkCreateBucketResult( AmazonS3 s3Client, String bucketName,
            String userName ) {
        // create one bucket,check the bucket name and owner name
        List<Bucket> buckets = s3Client.listBuckets();
        boolean findBucketFlag = false;
        for ( int i = 0; i < buckets.size(); i++ ) {
            String actBucketName = buckets.get( i ).getName();
            // get the create bucket,then check the bucket name and owner
            if ( actBucketName.equals( bucketName ) ) {
                Owner actOwner = buckets.get( i ).getOwner();
                Assert.assertEquals( actOwner.getDisplayName(), userName );
                findBucketFlag = true;
                break;
            }
        }
        Assert.assertTrue( findBucketFlag, " the bucket must be exist!" );
    }

    private void putObject( String bucketName, String objectName,
            String content, String authorization ) throws Exception {
        HttpPut request = new HttpPut( S3TestBase.s3ClientUrl + "/" + URLEncoder
                .encode( bucketName, "UTF-8" ) + "/" + URLEncoder
                .encode( objectName, "UTF-8" ) );
        // RequestHeaders:
        request.setHeader( "Authorization", authorization );

        // Requeatbody:
        StringEntity testString = new StringEntity( content,
                StandardCharsets.UTF_8 );
        request.setEntity( testString );
        CloseableHttpClient client = RestClient.createHttpClient();
        RestClient.sendRequest( client, request );
    }

    private String getObject( String bucketName, String objectName,
            String authorization ) throws Exception {
        String etag = "";
        HttpGet request = new HttpGet( S3TestBase.s3ClientUrl + "/" + URLEncoder
                .encode( bucketName, "UTF-8" ) + "/" + URLEncoder
                .encode( objectName, "UTF-8" ) );
        request.setHeader( "Authorization", authorization );
        CloseableHttpClient client = RestClient.createHttpClient();
        CloseableHttpResponse response = RestClient
                .sendRequest( client, request );
        etag = response.getFirstHeader( "ETag" ).getValue().replace( "\"", "" );

        return etag;
    }

    private void headObject( String bucketName, String objectName,
            String authorization ) throws Exception {
        HttpHead request = new HttpHead(
                S3TestBase.s3ClientUrl + "/" + URLEncoder
                        .encode( bucketName, "UTF-8" ) + "/" + URLEncoder
                        .encode( objectName, "UTF-8" ) );
        request.setHeader( "Authorization", authorization );
        CloseableHttpClient client = RestClient.createHttpClient();
        RestClient.sendRequest( client, request );
    }

    private void deleteObjet( String bucketName, String objectName,
            String authorization ) throws Exception {
        HttpDelete request = new HttpDelete(
                S3TestBase.s3ClientUrl + "/" + URLEncoder
                        .encode( bucketName, "UTF-8" ) + "/" + URLEncoder
                        .encode( objectName, "UTF-8" ) );
        request.setHeader( "Authorization", authorization );
        CloseableHttpClient client = RestClient.createHttpClient();
        RestClient.sendRequest( client, request );
    }

    private AmazonS3Exception httpToAmazonHead( HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        return amazonS3Exception;
    }
}
