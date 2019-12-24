package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CannedAccessControlList;
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
 * @Description seqDB-19461:配置和获取删除标记对象acl
 * @Author wangkexin
 * @Date 2019.09.23
 */
public class SetObjectAcl19461 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user19461";
    private String roleName = "normal";
    private String bucketName = "bucket19461";
    private String keyName = "key19461";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        // 创建一个用户
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        userS3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        ownerS3Client = CommLib.buildS3Client();
        CommLib.clearBucket( ownerS3Client, bucketName );
        ownerS3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( ownerS3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        ownerS3Client.deleteObject( bucketName, keyName );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        // 使用标准acl配置删除标记对象acl为public-read-wirte（不指定versionId）
        try {
            ownerS3Client.setObjectAcl( bucketName, keyName,
                    CannedAccessControlList.PublicReadWrite );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "MethodNotAllowed" ) ) {
                throw e;
            }
        }

        // 使用标准acl配置删除标记对象acl为public-read（指定versionId）
        try {
            ownerS3Client.setObjectAcl( bucketName, keyName, "0",
                    CannedAccessControlList.PublicRead );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "MethodNotAllowed" ) ) {
                throw e;
            }
        }

        // 使用标准acl获取删除标记对象acl（不指定versionId）
        try {
            ownerS3Client.getObjectAcl( bucketName, keyName );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "NoSuchKey" ) ) {
                throw e;
            }
        }

        // 使用标准acl获取删除标记对象acl（指定versionId）
        try {
            ownerS3Client.getObjectAcl( bucketName, keyName, "0" );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "MethodNotAllowed" ) ) {
                throw e;
            }
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( ownerS3Client, bucketName );
                CommLib.clearUser( userName );
            }
        } finally {
            ownerS3Client.shutdown();
            userS3Client.shutdown();
        }
    }
}
