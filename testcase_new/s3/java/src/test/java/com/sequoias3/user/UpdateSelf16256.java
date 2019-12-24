package com.sequoias3.user;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.json.JSONException;
import org.json.JSONObject;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @Description: seqDB-16256 :: 管理员更新自身
 * @author fanyu
 * @Date:2018年10月29日
 * @version:1.0
 */

public class UpdateSelf16256 extends S3TestBase {
    private String userName = "UpdateSelf16256";
    private String bucketName = "bucket16256";
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
    }

    @Test
    private void test() throws JSONException {
        // create user
        JSONObject createUser = UserUtils
                .createUser( userName, UserCommDefind.admin,
                        UserUtils.accessKeyId );

        // update user
        String accessKeyId = createUser
                .getJSONObject( UserCommDefind.accessKeys )
                .getString( UserCommDefind.accessKeyID );
        JSONObject updateUser = UserUtils.updateUser( userName, accessKeyId );

        // create bucket for check
        checkResult( createUser, updateUser );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        }
    }

    private void checkResult( JSONObject createUser, JSONObject updateUser ) {
        JSONObject createJSON = createUser
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID1 = createJSON
                .getString( UserCommDefind.accessKeyID );
        String secretAccessKey1 = createJSON
                .getString( UserCommDefind.secretAccessKey );

        JSONObject updateJSON = updateUser
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = updateJSON.getString( UserCommDefind.accessKeyID );
        String secretAccessKey = updateJSON
                .getString( UserCommDefind.secretAccessKey );

        // check accessKeyID and secretAccessKey was already updated
        Assert.assertNotEquals( accessKeyID1, accessKeyID );
        Assert.assertNotEquals( secretAccessKey1, secretAccessKey );

        // check updated accessKeyID and secretAccessKey is active
        AmazonS3 s3Client = null;
        try {
            s3Client = CommLib.buildS3Client( accessKeyID, secretAccessKey );
            // create bucket
            s3Client.createBucket( bucketName );

            // check
            List<Bucket> buckets = s3Client.listBuckets();
            Assert.assertEquals( buckets.size(), 1, " only one bucket" );
            Bucket expbucket = buckets.get( 0 );
            String actOwner = expbucket.getOwner().getDisplayName();
            String actBucketName = expbucket.getName();
            Assert.assertEquals( actBucketName, bucketName );
            Assert.assertEquals( actOwner, userName.toLowerCase() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
