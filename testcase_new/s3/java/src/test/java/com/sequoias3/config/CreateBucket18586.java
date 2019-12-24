package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.Owner;
import com.amazonaws.util.DateUtils;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: 开启鉴权，执行桶管理操作 testlink-case: seqDB-18586
 *
 * @author wangkexin
 * @Date 2019.06.20
 * @version 1.00
 */
public class CreateBucket18586 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String userName = "user18586";
    private int keyNum = 5;
    private String roleName = "normal";
    private String keyName = "key18586";
    private String regionName = "region18586";
    private AmazonS3 s3Client = null;
    private String[] accessKeys = null;

    private static List<Bucket> listBuckets( String authorization ) {
        TestRest rest = new TestRest();
        ResponseEntity<?> resp;
        List<Bucket> buckets = new ArrayList<>();
        try {
            resp = rest.setApi( "" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject jsonBody = XML.toJSONObject( xmlBody );
            JSONObject subjsonBody = jsonBody
                    .getJSONObject( "ListAllMyBucketsResult" );
            Object objects = subjsonBody.get( "Buckets" );
            Owner owner = new Owner();
            JSONObject ownerjsonBody = subjsonBody.getJSONObject( "Owner" );
            owner.setDisplayName( ownerjsonBody.getString( "DisplayName" ) );
            owner.setId( String.valueOf( ownerjsonBody.getInt( "ID" ) ) );
            if ( objects instanceof JSONObject ) {
                JSONObject jsonObject = ( JSONObject ) objects;
                Object jsonObjectBucket = jsonObject.get( "Bucket" );
                if ( jsonObjectBucket instanceof JSONArray ) {
                    JSONArray jsonArray = ( JSONArray ) jsonObjectBucket;
                    for ( int i = 0; i < jsonArray.length(); i++ ) {
                        Bucket bucket = new Bucket();
                        JSONObject subjsonObject = jsonArray.getJSONObject( i );
                        bucket.setName( subjsonObject.getString( "Name" ) );
                        bucket.setCreationDate( DateUtils.parseISO8601Date(
                                subjsonObject.getString( "CreationDate" ) ) );
                        bucket.setOwner( owner );
                        buckets.add( bucket );
                    }
                } else {
                    JSONObject json = ( JSONObject ) jsonObjectBucket;
                    Bucket bucket = new Bucket();
                    bucket.setName( json.getString( "Name" ) );
                    bucket.setCreationDate( DateUtils.parseISO8601Date(
                            json.getString( "CreationDate" ) ) );
                    bucket.setOwner( owner );
                    buckets.add( bucket );
                }
            }
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return buckets;
    }

    private static String getBucketLocation( String bucketName,
            String authorization ) {
        TestRest rest = new TestRest();
        ResponseEntity<?> resp;
        try {
            resp = rest.setApi( bucketName + "/?location" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject bucketListJSON = XML.toJSONObject( xmlBody );
            return bucketListJSON.getString( "LocationConstraint" );
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    @DataProvider(name = "authorizationProvider")
    public Object[][] generateAuthorization() {
        return new Object[][] {
                // test a : authorization 为version2版本
                new Object[] { "bucket18586v2",
                        "AWS " + accessKeys[ 0 ] + ":signature" },
                // test b : authorization 为version4版本
                new Object[] { "bucket18586v4",
                        UserCommDefind.authValPre + accessKeys[ 0 ] + "/" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        RegionUtils.clearRegion( regionName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );

        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );
    }

    @SuppressWarnings("deprecation")
    @Test(dataProvider = "authorizationProvider")
    private void testCreateBucket( String bucketName, String authorization )
            throws Exception {
        String tmpContent = "content18586" + authorization;
        // create bucket
        createBucket( bucketName, regionName, authorization );

        // check head bucket
        headBucket( bucketName, authorization );

        List<Bucket> buckets = listBuckets( authorization );
        for ( Bucket bucket : buckets ) {
            String actUserName = bucket.getOwner().getDisplayName();
            Assert.assertEquals( actUserName, userName );
            String actBucketName = bucket.getName();
            Assert.assertEquals( actBucketName, bucketName );
        }

        // check region info
        String location = getBucketLocation( bucketName, authorization );
        Assert.assertEquals( location, regionName );

        setBucketVersioning( authorization, bucketName,
                BucketVersioningConfiguration.ENABLED );
        String status = getBucketVersioning( authorization, bucketName );
        Assert.assertEquals( status, BucketVersioningConfiguration.ENABLED );

        List<String> contentList = new ArrayList<>();
        for ( int i = 0; i < keyNum; i++ ) {
            String currentExpContent = tmpContent + "." + i;
            putObject( bucketName, keyName, currentExpContent, authorization );
            contentList.add( currentExpContent );
        }
        checkPutObjectResult( authorization, bucketName, contentList );
        clearBucket( authorization, bucketName );
        Assert.assertFalse( s3Client.doesBucketExist( bucketName ) );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generateAuthorization().length ) {
                UserUtils.deleteUser( userName );
                RegionUtils.deleteRegion( regionName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void createBucket( String bucketName, String regionName,
            String authorization ) throws UnsupportedEncodingException {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( URLEncoder.encode( bucketName, "UTF-8" ) )
                    .setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .setRequestBody(
                            "<CreateBucketConfiguration><LocationConstraint>"
                                    + regionName
                                    + "</LocationConstraint></CreateBucketConfiguration>" )
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

    private void setBucketVersioning( String authorization, String bucketName,
            String versioning ) {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( bucketName + "/?versioning" )
                    .setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestBody(
                    "<VersioningConfiguration><Status>" + versioning
                            + "</Status></VersioningConfiguration>" )
                    .setResponseType( String.class ).exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private String getBucketVersioning( String authorization,
            String bucketName ) {
        TestRest rest = new TestRest( type );
        String status = "";
        try {
            ResponseEntity<?> resp = rest.setApi( bucketName + "/?versioning" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.GET )
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

    private void checkPutObjectResult( String authorization, String bucketName,
            List<String> contentList ) throws Exception {
        // Objects in the version list are stored in reverse order by versionId
        Collections.reverse( contentList );
        JSONArray version = listVersions( authorization, bucketName );
        // check object content by md5
        for ( int i = 0; i < version.length(); i++ ) {
            Assert.assertEquals(
                    version.getJSONObject( i ).getInt( "VersionId" ),
                    ( keyNum - 1 ) - i,
                    "versionid is wrong! version : " + version.toString() );
            String actMd5 = version.getJSONObject( i ).getString( "ETag" );
            Assert.assertEquals( actMd5,
                    TestTools.getMD5( contentList.get( i ).getBytes() ),
                    "md5 is different! content : " + contentList.get( i )
                            + ", version : " + version.toString() );
        }
    }

    private void clearBucket( String authorization, String bucketName )
            throws Exception {
        JSONArray version = listVersions( authorization, bucketName );
        for ( int i = 0; i < version.length(); i++ ) {
            deleteVersion( bucketName,
                    version.getJSONObject( i ).getString( "Key" ),
                    version.getJSONObject( i ).getInt( "VersionId" ),
                    authorization );
        }
        deleteBucket( bucketName, authorization );
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

    private void deleteVersion( String bucketName, String objectName,
            int versionId, String authorization ) throws Exception {
        HttpDelete request = new HttpDelete(
                S3TestBase.s3ClientUrl + "/" + URLEncoder
                        .encode( bucketName, "UTF-8" ) + "/" + URLEncoder
                        .encode( objectName, "UTF-8" ) + "?versionId="
                        + versionId );
        request.setHeader( "Authorization", authorization );
        CloseableHttpClient client = RestClient.createHttpClient();
        RestClient.sendRequest( client, request );
    }

    private JSONArray listVersions( String authorization, String bucketName ) {
        TestRest rest = new TestRest();
        ResponseEntity<?> resp = rest.setApi( bucketName + "/?versions" )
                .setRequestHeaders( UserCommDefind.authorization,
                        authorization ).setRequestMethod( HttpMethod.GET )
                .setResponseType( String.class ).exec();

        String body = resp.getBody().toString();
        JSONObject objectVersioningJSON = XML.toJSONObject( body );
        JSONObject listVersionsResult = objectVersioningJSON
                .getJSONObject( "ListVersionsResult" );
        JSONArray version = listVersionsResult.getJSONArray( "Version" );
        return version;
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

    private AmazonS3Exception httpToAmazonHead( HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        return amazonS3Exception;
    }
}
