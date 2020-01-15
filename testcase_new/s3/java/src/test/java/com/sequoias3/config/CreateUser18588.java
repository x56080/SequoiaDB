package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
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

/**
 * test content: 开启鉴权，使用默认用户执行用户管理操作 testlink-case: seqDB-18588
 *
 * @author wangkexin
 * @Date 2019.06.21
 * @version 1.00
 */
public class CreateUser18588 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private String userName1 = "user18588v4";
    private String userName2 = "user18588v2";
    private String keyName = "key18588";
    private String content = "content18588";

    @DataProvider(name = "authorizationProvider")
    public Object[][] generateAuthorization() {
        return new Object[][] {
                // test a : authorization 为version2版本
                new Object[] { "bucket18588v2", userName1,
                        UserCommDefind.normal,
                        "AWS " + UserUtils.accessKeyId + ":signature" },
                // test b : authorization 为version4版本
                new Object[] { "bucket18588v4", userName2, UserCommDefind.admin,
                        UserCommDefind.authValPre + UserUtils.accessKeyId
                                + "/" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName1 );
        CommLib.clearUser( userName2 );
    }

    @Test(dataProvider = "authorizationProvider")
    private void testCreateUser( String bucketName, String userName,
            String role, String authorization ) throws Exception {
        // create user
        String[] olbAccessKeys = createUser( userName, role, authorization );
        // update user
        String[] newAccessKeys = updateUser( userName, authorization );
        // create bucket for check
        checkResult( olbAccessKeys, newAccessKeys, bucketName, content );
        deleteUser( userName, authorization, true );

        checkDeletedUser( userName, authorization );
    }

    @AfterClass
    private void tearDown() throws Exception {
    }

    private void checkResult( String[] olbAccessKeys, String[] newAccessKeys,
            String bucketName, String content ) {
        // check accessKeyID and secretAccessKey was already updated
        Assert.assertNotEquals( olbAccessKeys[ 0 ], newAccessKeys[ 0 ] );
        Assert.assertNotEquals( olbAccessKeys[ 1 ], newAccessKeys[ 1 ] );

        // check updated accessKeyID and secretAccessKey is active
        AmazonS3 s3Client = null;
        try {
            s3Client = CommLib
                    .buildS3Client( newAccessKeys[ 0 ], newAccessKeys[ 1 ] );
            // create bucket
            s3Client.createBucket( bucketName );
            s3Client.putObject( bucketName, keyName, content );
            ObjectMetadata metadata = s3Client
                    .getObjectMetadata( bucketName, keyName );
            Assert.assertEquals( metadata.getETag(),
                    TestTools.getMD5( content.getBytes() ) );

        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkDeletedUser( String name, String authorization ) {
        try {
            getUser( name, authorization );
            Assert.fail( "exp fail but act success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUser" );
        }
    }

    private String[] createUser( String name, String role,
            String authorization ) {
        TestRest rest = new TestRest( type );
        try {
            ResponseEntity<?> resp = rest.setApi(
                    "/users/?Action=CreateUser&UserName=" + name + "&Role="
                            + role ).setRequestMethod( HttpMethod.POST )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
            String xmlBody = resp.getBody().toString();
            JSONObject resultJson = XML.toJSONObject( xmlBody );
            JSONObject AccessKeys = resultJson.getJSONObject( "AccessKeys" );
            String accessKeyID = AccessKeys
                    .getString( UserCommDefind.accessKeyID );
            String secretAccessKey = AccessKeys
                    .getString( UserCommDefind.secretAccessKey );
            return new String[] { accessKeyID, secretAccessKey };
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private String[] updateUser( String name, String authorization ) {
        TestRest rest = new TestRest( type );
        try {
            ResponseEntity<?> resp = rest
                    .setApi( "/users/?Action=CreateAccessKey&UserName=" + name )
                    .setRequestMethod( HttpMethod.POST )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
            String xmlBody = resp.getBody().toString();
            JSONObject resultJson = XML.toJSONObject( xmlBody );
            JSONObject AccessKeys = resultJson.getJSONObject( "AccessKeys" );
            String accessKeyID = AccessKeys
                    .getString( UserCommDefind.accessKeyID );
            String secretAccessKey = AccessKeys
                    .getString( UserCommDefind.secretAccessKey );
            return new String[] { accessKeyID, secretAccessKey };
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private void deleteUser( String name, String authorization,
            boolean force ) {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi(
                    "/users/?Action=DeleteUser&UserName=" + name + "&Force="
                            + force ).setRequestMethod( HttpMethod.POST )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private String getUser( String name, String authorization )
            throws HttpClientErrorException {
        TestRest rest = new TestRest( type );
        String accessKeyId = "";
        try {
            ResponseEntity<?> resp = rest
                    .setApi( "/users/?Action=GetAccessKey&UserName=" + name )
                    .setRequestMethod( HttpMethod.POST )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
            String xmlBody = resp.getBody().toString();
            JSONObject resultJson = XML.toJSONObject( xmlBody );
            JSONObject AccessKeys = resultJson.getJSONObject( "AccessKeys" );
            accessKeyId = AccessKeys.getString( "AccessKeyID" );
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return accessKeyId;
    }
}
