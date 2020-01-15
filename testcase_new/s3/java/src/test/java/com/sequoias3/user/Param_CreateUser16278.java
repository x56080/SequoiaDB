package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

import org.json.JSONObject;
import org.json.XML;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Random;

/**
 * @Description: seqDB-16278 ::POST USER参数校验
 * @author fanyu
 * @Date:2018年10月29日
 * @version:1.0
 */
public class Param_CreateUser16278 extends S3TestBase {

    @BeforeClass
    private void setUp() {
    }

    @Test
    private void testNameIsEmpty() {
        String name = "";
        JSONObject userJSON = null;
        try {
            userJSON = UserUtils.createUser( name, UserCommDefind.admin,
                    UserUtils.accessKeyId );
            Assert.fail( "exp failed but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json = XML.toJSONObject( errorMsg )
                    .getJSONObject( UserCommDefind.error );
            if ( !json.getString( UserCommDefind.errorCode )
                    .contains( "InvalidUserName" ) ) {
                Assert.fail( e.getResponseBodyAsString() );
            }
        } finally {
            if ( userJSON != null ) {
                UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            }
        }
    }

    @Test
    private void testNameIsSepcial() {
        // + means backspace
        String name = ".B=,ABCDEMN@_-adcdefghijklmnopqrstuvwxyzOPQRSTUVWXYZ012345789";
        JSONObject actUser = null;
        try {
            // create
            actUser = UserUtils.createUser( name, UserCommDefind.admin,
                    UserUtils.accessKeyId );
            // update
            JSONObject updateUser = UserUtils
                    .updateUser( name, UserUtils.accessKeyId );
            // get
            JSONObject expUser = UserUtils
                    .getUser( name, UserUtils.accessKeyId );
            checkResult( updateUser, expUser );
        } finally {
            if ( actUser != null ) {
                UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            }
        }
    }

    @Test
    private void testNameLenEq64() {
        String name = getRandomString( 64 );
        JSONObject actUser = null;
        try {
            // create
            actUser = UserUtils.createUser( name, UserCommDefind.admin,
                    UserUtils.accessKeyId );
            // update
            JSONObject updateUser = UserUtils
                    .updateUser( name, UserUtils.accessKeyId );
            // get
            JSONObject expUser = UserUtils
                    .getUser( name, UserUtils.accessKeyId );
            checkResult( updateUser, expUser );
        } finally {
            if ( actUser != null ) {
                UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            }
        }
    }

    @Test
    private void testNameLenGt64() {
        String name = getRandomString( 65 );
        JSONObject actUser = null;
        try {
            // create
            actUser = UserUtils.createUser( name, UserCommDefind.admin,
                    UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json = XML.toJSONObject( errorMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "InvalidUserName" ) ) {
                Assert.fail( e.getMessage() );
            }
        } finally {
            if ( actUser != null ) {
                UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            }
        }
    }

    @Test
    private void testNameSpecial() {
        String name = ".=,名字#￥%；@_-";
        JSONObject actUser = null;
        try {
            // create
            actUser = UserUtils.createUser( name, UserCommDefind.admin,
                    UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json = XML.toJSONObject( errorMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "InvalidUserName" ) ) {
                Assert.fail( e.getMessage() );
            }
        } finally {
            if ( actUser != null ) {
                UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            }
        }
    }

    @Test
    private void testRoleInvalid() {
        String name = "16278";
        JSONObject actUser = null;
        try {
            // create
            actUser = UserUtils
                    .createUser( name, "admin|normal", UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json = XML.toJSONObject( errorMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "InvalidRole" ) ) {
                Assert.fail( e.getMessage() );
            }
        } finally {
            if ( actUser != null ) {
                UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            }
        }
    }

    @Test
    private void testAccessKeyIDInvalid() {
        String name = "16278_1";
        JSONObject actUser = null;
        try {
            // create
            actUser = UserUtils.createUser( name, UserCommDefind.admin,
                    UserUtils.accessKeyId + "123" );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json = XML.toJSONObject( errorMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "InvalidAccessKeyId" ) ) {
                Assert.fail( e.getMessage() );
            }
        } finally {
            if ( actUser != null ) {
                UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            }
        }
    }

    @AfterClass
    private void tearDown() {
    }

    private void checkResult( JSONObject actUser, JSONObject expUser ) {
        Assert.assertEquals( actUser.getJSONObject( UserCommDefind.accessKeys )
                        .getString( UserCommDefind.accessKeyID ),
                expUser.getJSONObject( UserCommDefind.accessKeys )
                        .getString( UserCommDefind.accessKeyID ) );

        Assert.assertEquals( actUser.getJSONObject( UserCommDefind.accessKeys )
                        .getString( UserCommDefind.secretAccessKey ),
                expUser.getJSONObject( UserCommDefind.accessKeys )
                        .getString( UserCommDefind.secretAccessKey ) );
    }

    private String getRandomString( int length ) {
        String str = "adcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int number = random.nextInt( str.length() );
            sb.append( str.charAt( number ) );
        }
        return sb.toString();
    }
}
