package com.sequoias3.user;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
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
 * @Description: seqDB-16263 :: 管理员删除用户后，再次创建同名用户
 * @author fanyu
 * @Date:2018年10月30日
 * @version:1.0
 */

public class DeleteThenCreate16263 extends S3TestBase {
    private String userName = "DeleteThenCreate16263";
    private String bucketName = "bucket16263";
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
    private void test() {
        // create user
        UserUtils.createUser( userName, UserCommDefind.admin,
                UserUtils.accessKeyId );

        // delete user
        UserUtils.deleteUser( userName, UserUtils.accessKeyId );

        // check
        checkDelete();

        // create same user,the type is different
        JSONObject userJSON = UserUtils
                .createUser( userName, UserCommDefind.normal,
                        UserUtils.accessKeyId );
        JSONObject json = userJSON.getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = json.getString( UserCommDefind.accessKeyID );
        String secretAccessKey = json
                .getString( UserCommDefind.secretAccessKey );

        // check the user is active
        createBucketAndCheck( accessKeyID, secretAccessKey );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        }
    }

    private void checkDelete() {
        try {
            UserUtils.getUser( userName, UserUtils.accessKeyId );
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

    private void createBucketAndCheck( String accessKeyID,
            String secretAccessKey ) {
        AmazonS3 s3Client = null;
        try {
            s3Client = CommLib.buildS3Client( accessKeyID, secretAccessKey );
            // create bucket
            s3Client.createBucket( bucketName.toLowerCase() );

            // check
            List<Bucket> buckets = s3Client.listBuckets();
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
}
