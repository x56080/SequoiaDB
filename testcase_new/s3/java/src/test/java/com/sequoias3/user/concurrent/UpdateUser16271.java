package com.sequoias3.user.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.json.JSONObject;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description: seqDB-16271 :: 并发更新相同用户
 * @author wangkexin
 * @Date:2018年11月06日
 * @version:1.0
 */

public class UpdateUser16271 extends S3TestBase {
    private String username = "UpdateUser16271";
    private String bucketName = "bucket16271";
    private int num = 100;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        try {
            UserUtils.deleteUser( username, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                Assert.fail( e.getMessage() );
            }
        }
        // create an normal user
        UserUtils.createUser( username, UserCommDefind.normal,
                UserUtils.accessKeyId );
    }

    @Test
    public void testUpdateUser() throws Exception {
        List<UpdateUser> threads = new ArrayList<UpdateUser>();
        for ( int i = 0; i < num; i++ ) {
            threads.add( new UpdateUser() );
        }
        for ( int i = 0; i < num; i++ ) {
            threads.get( i ).start();
        }
        for ( int i = 0; i < num; i++ ) {
            Assert.assertTrue( threads.get( i ).isSuccess(),
                    threads.get( i ).getErrorMsg() );
        }
        // check result
        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            UserUtils.deleteUser( username, UserUtils.accessKeyId, true );
        }
    }

    private void checkResult() {
        JSONObject updateUser = UserUtils
                .getUser( username, UserUtils.accessKeyId );
        JSONObject updateJSON = updateUser
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = updateJSON.getString( UserCommDefind.accessKeyID );
        String secretAccessKey = updateJSON
                .getString( UserCommDefind.secretAccessKey );

        // check updated accessKeyID and secretAccessKey is active
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
            Assert.assertEquals( actOwner, username.toLowerCase() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class UpdateUser extends S3ThreadBase {
        @Override
        public void exec() {
            try {
                UserUtils.updateUser( username, UserUtils.accessKeyId );
            } catch ( HttpClientErrorException e ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }
}
