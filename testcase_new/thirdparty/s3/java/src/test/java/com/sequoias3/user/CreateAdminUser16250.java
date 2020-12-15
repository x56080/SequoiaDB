package com.sequoias3.user;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @author fanyu
 * @Description: seqDB-16250 :: 管理员创建管理员（admin）用户
 * @Date:2018年10月29日
 * @version:1.0
 */

public class CreateAdminUser16250 extends S3TestBase {
    private String userName = "CreateAdminUser16250";
    private String bucketName = "bucket16250";
    private String accessKeyID = null;
    private String secretAccessKey = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    private void test() throws JSONException {
        // create user
        JSONObject actUser = UserUtils.createUser( userName,
                UserCommDefind.admin, UserUtils.accessKeyId );
        // CRUD user for check
        checkByCRUDUser( actUser );
        // create bucket for check
        checkByCreateBucket();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        }
    }

    private void checkByCRUDUser( JSONObject admin ) {
        // get the accessKeyID and secretAccessKey from admin
        JSONObject adminJSON = admin.getJSONObject( UserCommDefind.accessKeys );
        accessKeyID = adminJSON.getString( UserCommDefind.accessKeyID );
        secretAccessKey = adminJSON.getString( UserCommDefind.secretAccessKey );

        String username = userName + "_16250";
        JSONObject actJSON = createAndCheck( username, accessKeyID );
        updateAndCheck( username, actJSON, accessKeyID );
        deleteAndCheck( username, accessKeyID );
    }

    private void checkByCreateBucket() {
        // check the user is active
        AmazonS3 s3Client = null;
        try {
            s3Client = CommLib.buildS3Client( accessKeyID, secretAccessKey );
            // create bucket
            s3Client.createBucket( bucketName.toLowerCase() );
            // check
            List< Bucket > buckets = s3Client.listBuckets();
            Assert.assertEquals( buckets.size(), 1, " only one bucket" );
            Bucket expbucket = buckets.get( 0 );
            String actOwner = expbucket.getOwner().getDisplayName();
            String actBucketName = expbucket.getName();
            Assert.assertEquals( actBucketName, bucketName.toLowerCase() );
            Assert.assertEquals( actOwner, userName.toLowerCase() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private JSONObject createAndCheck( String username, String accessKeyID ) {
        // craete user
        JSONObject actUser = UserUtils.createUser( username,
                UserCommDefind.normal, accessKeyID );
        JSONObject actJSON = actUser.getJSONObject( UserCommDefind.accessKeys );
        // get user
        JSONObject expUser = UserUtils.getUser( username, accessKeyID );
        JSONObject expJSON = expUser.getJSONObject( UserCommDefind.accessKeys );
        // check
        Assert.assertEquals( actJSON.getString( UserCommDefind.accessKeyID ),
                expJSON.getString( UserCommDefind.accessKeyID ),
                "actJSON = " + actJSON.toString() + ",expJSON = "
                        + expJSON.toString() );
        Assert.assertEquals(
                actJSON.getString( UserCommDefind.secretAccessKey ),
                expJSON.getString( UserCommDefind.secretAccessKey ),
                "actJSON = " + actJSON.toString() + ",expJSON = "
                        + expJSON.toString() );
        return actJSON;
    }

    private void updateAndCheck( String username, JSONObject actJSON,
            String accessKeyID ) {
        // update user
        JSONObject updateUser = UserUtils.updateUser( username, accessKeyID );
        JSONObject upadteJSON = updateUser
                .getJSONObject( UserCommDefind.accessKeys );
        // check the user was updated
        Assert.assertNotEquals( actJSON.getString( UserCommDefind.accessKeyID ),
                upadteJSON.getString( UserCommDefind.accessKeyID ) );
        Assert.assertNotEquals(
                actJSON.getString( UserCommDefind.secretAccessKey ),
                upadteJSON.getString( UserCommDefind.secretAccessKey ) );

    }

    private void deleteAndCheck( String username, String accessKeyID ) {
        // delete user
        UserUtils.deleteUser( username, accessKeyID );
        // check user was deleted
        try {
            UserUtils.getUser( username, UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json1 = XML.toJSONObject( errorMsg );
            if ( !json1.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "NoSuchUser" ) ) {
                Assert.fail( e.getMessage() );
            }
        }
    }
}
