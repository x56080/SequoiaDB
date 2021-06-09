package com.sequoias3.object;

import com.amazonaws.ClientConfiguration;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.AWSStaticCredentialsProvider;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3ClientBuilder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.RequestEntity;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.RestTemplate;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.*;

/**
 * @Description: seqDB-24256:通过预签名获取url，使用head object方式访问
 *
 * @author YiPan
 * @Date 2021.6.7
 * @version 1.00
 */
public class GetObjectByGeneratePresignedUrl24256 extends S3TestBase {
    private static String AWS_ACCESS_KEY = "ABCDEFGHIJKLMNOPQRST";
    private static String AWS_SECRET_KEY = "abcdefghijklmnopqrstuvwxyz0123456789ABCD";
    private static String clientRegion = "us-east-1";
    private String bucketName = "bucket24256";
    private String keyName = "testkey_24256";
    private String file = "object24256";
    private AmazonS3 s3ClientV4 = null;
    private AmazonS3 s3ClientV2 = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        s3ClientV4 = CommLib.buildS3Client();
        s3ClientV2 = buildS3ClientV2();
        CommLib.clearBucket( s3ClientV4, bucketName );
        s3ClientV4.createBucket( bucketName );
        s3ClientV4.putObject( bucketName, keyName, file );
    }

    @Test
    public void test() throws Exception {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis( new Date().getTime() + 1000 * 10 );
        Date expireTime = calendar.getTime();
        URL urlv4 = s3ClientV4.generatePresignedUrl( bucketName, keyName,
                expireTime );
        URL urlv2 = s3ClientV2.generatePresignedUrl( bucketName, keyName,
                expireTime );
        checkResponse( urlv4 );
        checkResponse( urlv2 );
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.clearBucket( s3ClientV4, bucketName );
            s3ClientV4.shutdown();
            s3ClientV2.shutdown();
        }
    }

    private AmazonS3 buildS3ClientV2() {
        AmazonS3 s3Client = null;
        AWSCredentials credentials = new BasicAWSCredentials( AWS_ACCESS_KEY,
                AWS_SECRET_KEY );
        AwsClientBuilder.EndpointConfiguration endpointConfiguration = new AwsClientBuilder.EndpointConfiguration(
                S3TestBase.s3ClientUrl, clientRegion );
        ClientConfiguration config = new ClientConfiguration();
        config.setUseExpectContinue( false );
        config.setSocketTimeout( 300000 );
        // 以v2方式生成连接
        config.setSignerOverride( "S3SignerType" );
        s3Client = AmazonS3ClientBuilder.standard()
                .withEndpointConfiguration( endpointConfiguration )
                .withClientConfiguration( config )
                .withChunkedEncodingDisabled( true )
                .withPathStyleAccessEnabled( true )
                .withCredentials(
                        new AWSStaticCredentialsProvider( credentials ) )
                .build();
        return s3Client;
    }

    private void checkResponse( URL url ) throws URISyntaxException {
        RestTemplate restTemplate = new RestTemplate();
        RequestEntity< ? > requestEntity = new RequestEntity<>( HttpMethod.HEAD,
                new URI( url.toString() ) );
        ResponseEntity< Resource > resp = restTemplate.exchange( requestEntity,
                Resource.class );
        Assert.assertEquals( resp.getStatusCode(), HttpStatus.OK );
    }
}
