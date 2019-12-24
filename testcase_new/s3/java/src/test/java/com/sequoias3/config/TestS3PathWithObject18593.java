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
 * test content: 设置带s3路径校验，执行对象管理操作 testlink-case: seqDB-18593
 *
 * @author wangkexin
 * @Date 2019.06.25
 * @version 1.00
 */
public class TestS3PathWithObject18593 extends S3TestBase {
    private String addr = "";
    private boolean runSuccess = false;
    private String bucketName = "bucket18593";
    private String keyName = "key18593";
    private String content = "content18593";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        addr = "http://" + S3TestBase.s3HostName + ":" + S3TestBase.s3Port
                + "/s3/";
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void testS3Path() {
        try {
            putObject( bucketName, keyName, content );
            Assert.fail( "put object " + keyName + " should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }

        Assert.assertFalse( s3Client.doesObjectExist( bucketName, keyName ) );
        s3Client.putObject( bucketName, keyName, content );

        try {
            getObject( bucketName, keyName );
            Assert.fail( "get object " + keyName + " should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        try {
            headObject( bucketName, keyName );
            Assert.fail( "head object " + keyName + " should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        try {
            listOvjectV2( bucketName );
            Assert.fail( "list objects should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        try {
            listVersions( bucketName );
            Assert.fail( "list object versions should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        try {
            deleteObjet( bucketName, keyName );
            Assert.fail( "delete object should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        try {
            deleteVersion( bucketName, keyName, 0 );
            Assert.fail( "delete object version should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    public void putObject( String bucketName, String objectName,
            String content ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi( bucketName + "/" + objectName )
                    .setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setRequestBody( content )
                    .setResponseType( String.class ).exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    public String getObject( String bucketName, String objectName ) {
        ResponseEntity<?> resp;
        TestRest rest = new TestRest( addr );
        try {
            resp = rest.setApi( bucketName + "/" + objectName )
                    .setRequestMethod( HttpMethod.GET )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setResponseType( String.class )
                    .exec();
            return resp.getHeaders().getETag();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    public void headObject( String bucketName, String objectName ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi( bucketName + "/" + objectName )
                    .setRequestMethod( HttpMethod.HEAD )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setResponseType( String.class )
                    .exec();
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazonHead( e );
        }
    }

    public void deleteObjet( String bucketName, String objectName ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi( bucketName + "/" + objectName )
                    .setRequestMethod( HttpMethod.DELETE )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setResponseType( String.class )
                    .exec();
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazonHead( e );
        }
    }

    public void deleteVersion( String bucketName, String objectName,
            int versionId ) {
        TestRest rest = new TestRest( addr );
        try {
            rest.setApi(
                    bucketName + "/" + objectName + "?versionId=" + versionId )
                    .setRequestMethod( HttpMethod.DELETE )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setResponseType( String.class )
                    .exec();
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazonHead( e );
        }
    }

    public List<String> listOvjectV2( String bucketName ) {
        TestRest rest = new TestRest( addr );
        ResponseEntity<?> resp;
        Object contents = null;
        List<String> keyNames = new ArrayList<>();

        try {
            resp = rest.setApi( bucketName + "/?list-type=2" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject resultJson = XML.toJSONObject( xmlBody );
            JSONObject listBucketResultJson = resultJson
                    .getJSONObject( "ListBucketResult" );
            try {
                contents = listBucketResultJson.get( "Contents" );
            } catch ( JSONException e ) {
                Assert.assertEquals( e.getMessage(),
                        "JSONObject[\"Contents\"] not found." );
            }
            if ( contents != null ) {
                if ( contents instanceof JSONArray ) {
                    JSONArray array = ( JSONArray ) contents;
                    for ( int i = 0; i < array.length(); i++ ) {
                        keyNames.add(
                                array.getJSONObject( i ).getString( "Key" ) );
                    }
                } else {
                    keyNames.add(
                            ( ( JSONObject ) contents ).getString( "Key" ) );
                }
            }
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return keyNames;
    }

    public JSONArray listVersions( String bucketName ) {
        TestRest rest = new TestRest( addr );
        Object version = new JSONArray();
        try {
            ResponseEntity<?> resp = rest.setApi( bucketName + "/?versions" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + UserUtils.accessKeyId
                                    + "/" ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();

            String body = resp.getBody().toString();
            JSONObject objectVersioningJSON = XML.toJSONObject( body );
            JSONObject listVersionsResult = objectVersioningJSON
                    .getJSONObject( "ListVersionsResult" );

            try {
                version = listVersionsResult.get( "Version" );
            } catch ( JSONException e ) {
                Assert.assertEquals( e.getMessage(),
                        "JSONObject[\"Version\"] not found." );
            }
            if ( version instanceof JSONArray ) {
                return ( JSONArray ) version;
            } else {
                JSONArray array = new JSONArray();
                return array.put( version );
            }
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private AmazonS3Exception httpToAmazonHead( HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        return amazonS3Exception;
    }
}
