package com.sequoias3.user.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.json.JSONObject;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description: seqDB-16270 :: 并发创建用户
 * @author fanyu
 * @Date:2018年10月30日
 * @version:1.0
 */

public class CreateUser16270 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "CreateUser16720";
    private String bucketName = "bucket16270";
    private int num = 50;
    private List<JSONObject> userList = new CopyOnWriteArrayList<JSONObject>();

    @BeforeClass
    private void setUp() throws Exception {
        for ( int i = 0; i < num; i++ ) {
            try {
                UserUtils.deleteUser( userName + "." + i, UserUtils.accessKeyId,
                        true );
            } catch ( HttpClientErrorException e ) {
                if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                    Assert.fail( e.getMessage() );
                }
            }
        }
    }

    @Test
    public void testCreateBucket() throws Exception {
        List<CreateUser> threads = new ArrayList<CreateUser>();
        for ( int i = 0; i < num; i++ ) {
            threads.add(
                    new CreateUser( userName + "." + i, UserCommDefind.normal,
                            UserUtils.accessKeyId ) );
        }

        for ( int i = 0; i < num; i++ ) {
            threads.get( i ).start();
        }

        for ( int i = 0; i < num; i++ ) {
            Assert.assertTrue( threads.get( i ).isSuccess(),
                    threads.get( i ).getErrorMsg() );
        }

        // check result
        for ( JSONObject userJSON : userList ) {
            createBuckets( userJSON );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            for ( int i = 0; i < num; i++ ) {
                UserUtils.deleteUser( userName + "." + i, UserUtils.accessKeyId,
                        true );
            }
        }
    }

    private void createBuckets( JSONObject userJSON ) {
        JSONObject json1 = userJSON.getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = json1.getString( UserCommDefind.accessKeyID );
        String secretAccessKey = json1
                .getString( UserCommDefind.secretAccessKey );
        AmazonS3 s3Client = null;
        try {
            s3Client = CommLib.buildS3Client( accessKeyID, secretAccessKey );
            // create bucket
            s3Client.createBucket( bucketName + "-" + UUID.randomUUID() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    public class CreateUser extends S3ThreadBase {
        private String username;
        private String type;
        private String accessKeyID;

        public CreateUser( String username, String type, String accessKeyID ) {
            this.username = username;
            this.type = type;
            this.accessKeyID = accessKeyID;
        }

        @Override
        public void exec() {
            try {
                JSONObject userJSON = UserUtils
                        .createUser( username, type, accessKeyID );
                userList.add( userJSON );
            } catch ( Exception e ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }
}
