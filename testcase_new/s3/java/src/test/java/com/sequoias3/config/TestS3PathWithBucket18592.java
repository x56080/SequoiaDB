package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.HttpStatusCodeException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * test content: 设置带s3路径校验，执行桶管理操作 testlink-case: seqDB-18592
 *
 * @author wangkexin
 * @Date 2019.06.25
 * @version 1.00
 */
public class TestS3PathWithBucket18592 extends S3TestBase {
    private String addr = "";
    private boolean runSuccess = false;
    private String bucketName = "bucket18592";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        addr = "http://" + S3TestBase.s3HostName + ":" + S3TestBase.s3Port
                + "/s3/";
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
    }

    @Test
    private void testS3Path() {
        try {
            createBucket( bucketName );
            Assert.fail( "create bucket " + bucketName + " should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }

        Assert.assertFalse( s3Client.doesBucketExist( bucketName ) );
        s3Client.createBucket( bucketName );

        try {
            headBucket( bucketName );
            Assert.fail( "head bucket " + bucketName + " should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        try {
            setBucketVersioning( bucketName,
                    BucketVersioningConfiguration.ENABLED );
            Assert.fail( "set bucket " + bucketName
                    + " versioning should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }

        try {
            getBucketVersioning( bucketName );
            Assert.fail( "get bucket " + bucketName
                    + " versioning should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }

        try {
            listBuckets();
            Assert.fail( "list buckets should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }

        try {
            getBucketLocation( bucketName );
            Assert.fail( "get bucket location should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }

        try {
            deleteBucket( bucketName );
            Assert.fail( "delete bucket should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    public void createBucket( String bucketName ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi( bucketName ).setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setResponseType( String.class )
                    .exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    public void headBucket( String bucketName ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi( bucketName ).setRequestMethod( HttpMethod.HEAD )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setResponseType( String.class )
                    .exec();
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazonHead( e );
        }
    }

    public void deleteBucket( String bucketName ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi( bucketName ).setRequestMethod( HttpMethod.DELETE )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setResponseType( String.class )
                    .exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    public List<String> listBuckets() {
        TestRest rest = new TestRest( addr );
        ResponseEntity<?> resp;
        JSONObject buckets = null;
        try {
            List<String> bucketNames = new ArrayList<>();
            resp = rest.setApi( "" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject bucketListJSON = XML.toJSONObject( xmlBody );
            try {
                buckets = bucketListJSON
                        .getJSONObject( "ListAllMyBucketsResult" )
                        .getJSONObject( "Buckets" );
            } catch ( JSONException e ) {
                Assert.assertEquals( e.getMessage(),
                        "JSONObject[\"Buckets\"] not found." );
            }
            if ( buckets != null ) {
                Object bucket = buckets.get( "Bucket" );
                if ( bucket instanceof JSONArray ) {
                    JSONArray array = ( JSONArray ) bucket;
                    for ( int i = 0; i < array.length(); i++ ) {
                        bucketNames.add( array.getJSONObject( i )
                                .getString( "Name" ) );
                    }
                } else {
                    bucketNames.add( ( ( JSONObject ) bucket )
                            .getString( "Name" ) );
                }
            }
            return bucketNames;
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    public String getBucketLocation( String bucketName ) {
        TestRest rest = new TestRest( addr );
        ResponseEntity<?> resp;
        try {
            resp = rest.setApi( bucketName + "/?location" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject bucketListJSON = XML.toJSONObject( xmlBody );
            return bucketListJSON.getString( "LocationConstraint" );
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    public void setBucketVersioning( String bucketName, String versioning ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi( bucketName + "/?versioning" )
                    .setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setRequestBody(
                    "<VersioningConfiguration><Status>" + versioning
                            + "</Status></VersioningConfiguration>" )
                    .setResponseType( String.class ).exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    public String getBucketVersioning( String bucketName ) {
        TestRest rest = new TestRest( addr );
        String status = "";
        try {
            ResponseEntity<?> resp = rest.setApi( bucketName + "/?versioning" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();

            String body = resp.getBody().toString();
            JSONObject bucketVersioningJSON = XML.toJSONObject( body );
            status = bucketVersioningJSON
                    .getJSONObject( "VersioningConfiguration" )
                    .getString( "Status" );
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }

        return status;
    }

    private AmazonS3Exception httpToAmazonHead( HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        return amazonS3Exception;
    }
}
