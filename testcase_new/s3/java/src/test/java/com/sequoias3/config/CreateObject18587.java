package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.delimiter.DelimiterConfiguration;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
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
 * test content: 开启鉴权，执行对象管理操作 testlink-case: seqDB-18587
 *
 * @author wangkexin
 * @Date 2019.06.21
 * @version 1.00
 */
public class CreateObject18587 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String userName = "user18587";
    private String roleName = "normal";
    private String[] objectNames = { "dir1?test18587_1",
            "dir1?Dir2?test18587_2", "?aa?bb?test18587_3",
            "?aa?cc?test18587_4" };
    private String content = "content18587";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private String[] accessKeys = null;

    @DataProvider(name = "authorizationProvider", parallel = true)
    public Object[][] generateAuthorization() {
        return new Object[][] {
                // test a : authorization 为version2版本
                new Object[] { "bucket18587v2",
                        "AWS " + accessKeys[ 0 ] + ":signature" },
                // test b : authorization 为version4版本
                new Object[] { "bucket18587v4",
                        UserCommDefind.authValPre + accessKeys[ 0 ] + "/" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test(dataProvider = "authorizationProvider")
    private void testCreateObject( String bucketName, String authorization )
            throws Exception {
        // create bucket
        createBucket( bucketName, authorization );
        putBucketDelimiter( bucketName, delimiter, authorization );
        checkCurrentDelimiteInfo( bucketName, delimiter, authorization );

        for ( int i = 0; i < objectNames.length; i++ ) {
            String tmpContent = content + "_" + i;
            putObject( bucketName, objectNames[ i ], tmpContent,
                    authorization );
            String actMd5 = getObject( bucketName, objectNames[ i ],
                    authorization );
            Assert.assertEquals( actMd5,
                    TestTools.getMD5( tmpContent.getBytes() ),
                    "get object, check etag is wrong, key = "
                            + objectNames[ i ] );
            headObject( bucketName, objectNames[ i ], authorization );
        }

        // check
        List<String> expContentList = new ArrayList<>();
        List<String> expCommonPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        JSONObject ListBucketResultObj = listObjectsWithDelimiter( bucketName,
                delimiter, authorization );
        checkListObjV2WithDelimiter( ListBucketResultObj, expCommonPrefixes,
                expContentList );

        clearBucket( bucketName, authorization );
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

    private void putBucketDelimiter( String bucketName, String delimiter,
            String authorization ) {
        DelimiterConfiguration delimiterConfig = new DelimiterConfiguration();
        delimiterConfig.setDelimiter( delimiter );
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( bucketName + "/?delimiter-config" )
                    .setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestBody( delimiterConfig )
                    .setResponseType( String.class ).exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private void checkCurrentDelimiteInfo( String bucketName, String delimiter,
            String authorization ) {
        DelimiterConfiguration delimiterResult = getDelimiter( bucketName,
                authorization );
        String curDelimiter = delimiterResult.getDelimiter();
        String curStatus = delimiterResult.getStatus();
        Assert.assertEquals( curDelimiter, delimiter );
        Assert.assertEquals( curStatus, "Normal" );
    }

    private DelimiterConfiguration getDelimiter( String bucketName,
            String authorization ) {
        TestRest rest = new TestRest( type );
        ResponseEntity<?> resp;
        DelimiterConfiguration result;
        try {
            resp = rest.setApi( bucketName + "/?delimiter-config" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject jsonBody = XML.toJSONObject( xmlBody );
            JSONObject subjsonBody = jsonBody
                    .getJSONObject( "DelimiterConfiguration" );
            String delimiter = subjsonBody.getString( "Delimiter" );
            String status = subjsonBody.getString( "Status" );
            result = new DelimiterConfiguration( delimiter, status );

        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return result;
    }

    private void clearBucket( String bucketName, String authorization )
            throws Exception {
        List<String> keyNames = listObjects( bucketName, authorization );
        for ( String key : keyNames ) {
            deleteObjet( bucketName, key, authorization );
            try {
                headObject( bucketName, key, authorization );
                Assert.fail( "object " + key + " is still exists!" );
            } catch ( Exception e ) {
                String errorcode = e.getMessage().substring( 8, 11 );
                Assert.assertEquals( errorcode, "404", e.getMessage() );
            }
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

    private JSONObject listObjectsWithDelimiter( String bucketName,
            String delimiter, String authorization ) {
        TestRest rest = new TestRest();
        ResponseEntity<?> resp;
        JSONObject listBucketResultJson = null;

        try {
            resp = rest.setApi(
                    bucketName + "/?list-type=2&delimiter=" + delimiter )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject resultJson = XML.toJSONObject( xmlBody );
            listBucketResultJson = resultJson
                    .getJSONObject( "ListBucketResult" );

        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return listBucketResultJson;
    }

    private void checkListObjV2WithDelimiter( JSONObject ListBucketResultObj,
            List<String> expCommonPrefixes, List<String> expContents ) {

        List<String> actCommprefixes = new ArrayList<String>();
        List<String> actContents = new ArrayList<String>();
        JSONArray commonPrefixes = new JSONArray();
        JSONArray contents = new JSONArray();

        try {
            commonPrefixes = ListBucketResultObj
                    .getJSONArray( "CommonPrefixes" );
        } catch ( JSONException e ) {
            Assert.assertEquals( e.getMessage(),
                    "JSONObject[\"CommonPrefixes\"] not found." );
        }
        for ( int i = 0; i < commonPrefixes.length(); i++ ) {
            actCommprefixes.add( commonPrefixes.getJSONObject( i )
                    .getString( "Prefix" ) );
        }

        try {
            contents = ListBucketResultObj.getJSONArray( "Contents" );
        } catch ( JSONException e ) {
            Assert.assertEquals( e.getMessage(),
                    "JSONObject[\"Contents\"] not found." );
        }
        for ( int i = 0; i < contents.length(); i++ ) {
            actContents.add( contents.getJSONObject( i ).getString( "Key" ) );
        }
        Collections.sort( expCommonPrefixes );
        Collections.sort( actCommprefixes );
        Assert.assertEquals( actCommprefixes, expCommonPrefixes,
                "actPrefixes:" + actCommprefixes.toString() + "\n expPrefixes:"
                        + expCommonPrefixes.toString() );

        Collections.sort( expContents );
        Collections.sort( actContents );
        Assert.assertEquals( actContents, expContents,
                "actcontents: " + actContents.toString() + ", expcontents: "
                        + expContents.toString() );

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

    private List<String> listObjects( String bucketName,
            String authorization ) {
        TestRest rest = new TestRest();
        ResponseEntity<?> resp;
        JSONArray contents = new JSONArray();
        List<String> keyNames = new ArrayList<>();

        try {
            resp = rest.setApi( bucketName + "/?list-type=2" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject resultJson = XML.toJSONObject( xmlBody );
            JSONObject listBucketResultJson = resultJson
                    .getJSONObject( "ListBucketResult" );
            try {
                contents = listBucketResultJson.getJSONArray( "Contents" );
            } catch ( JSONException e ) {
                Assert.assertEquals( e.getMessage(),
                        "JSONObject[\"Contents\"] not found." );
            }

            for ( int i = 0; i < contents.length(); i++ ) {
                keyNames.add( contents.getJSONObject( i ).getString( "Key" ) );
            }
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }

        return keyNames;
    }
}
