package com.sequoias3.config;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
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

import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: 开启鉴权，使用普通用户执行用户管理操作 testlink-case: seqDB-18589
 *
 * @author wangkexin
 * @Date 2019.06.24
 * @version 1.00
 */
public class CreateUser18589 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String userName = "normaluser18589";
    private String roleName = "normal";
    private String[] accessKeys = null;

    @DataProvider(name = "authorizationProvider")
    public Object[][] generateAuthorization() {
        return new Object[][] {
                // test a : authorization 为version2版本
                new Object[] { UserCommDefind.normal,
                        "AWS " + accessKeys[ 0 ] + ":signature" },
                // test b : authorization 为version4版本
                new Object[] { UserCommDefind.admin,
                        UserCommDefind.authValPre + accessKeys[ 0 ] + "/" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
    }

    @Test(dataProvider = "authorizationProvider")
    private void testCreateUserV2( String role, String authorization )
            throws Exception {
        String userName = "user18589";
        try {
            createUser( userName, role, authorization );
            Assert.fail( "create " + role
                    + " user by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        try {
            updateUser( userName, authorization );
            Assert.fail( "update " + role
                    + " user by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        try {
            getUser( userName, authorization );
            Assert.fail(
                    "get " + role + " user by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        try {
            deleteUser( userName, authorization, false );
            Assert.fail( "delete " + role
                    + " user by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( actSuccessTests.get() == generateAuthorization().length ) {
            UserUtils.deleteUser( userName );
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
