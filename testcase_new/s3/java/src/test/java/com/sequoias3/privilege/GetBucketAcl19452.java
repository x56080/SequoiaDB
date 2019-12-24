package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-19452: 无桶访问权限的用户获取桶acl
 * @Author wangkexin
 * @Date 2019.09.23
 */
public class GetBucketAcl19452 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user19452";
    private String roleName = "normal";
    private String bucketName = "bucket19452";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        // 创建用户
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        userS3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        ownerS3Client = CommLib.buildS3Client();
        CommLib.clearBucket( ownerS3Client, bucketName );
        // 管理员用户创建桶
        ownerS3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void testSetBucketAcl() throws Exception {
        // 使用无权限普通用户获取桶acl
        try {
            userS3Client.getBucketAcl( bucketName );
            Assert.fail(
                    "Users who are not bucket owner should fail to get bucket acl." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "AccessDenied" ) ) {
                throw e;
            }
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ownerS3Client.deleteBucket( bucketName );
                CommLib.clearUser( userName );
            }
        } finally {
            ownerS3Client.shutdown();
            userS3Client.shutdown();
        }
    }
}
