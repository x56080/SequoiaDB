package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-19448: 配置桶acl为私有，更新桶acl
 * @Author wangkexin
 * @Date 2019.09.19
 */
public class SetBucketAcl19448 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user19448";
    private String roleName = "normal";
    private String bucketName = "bucket19448";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;
    private String ownerId;

    @BeforeClass
    private void setUp() throws IOException {
        // 创建用户
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        userS3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        ownerS3Client = CommLib.buildS3Client();
        ownerId = ownerS3Client.getS3AccountOwner().getId();
        CommLib.clearBucket( ownerS3Client, bucketName );
        ownerS3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void testSetBucketAcl() throws Exception {
        // 使用标准acl配置桶acl为private
        ownerS3Client
                .setBucketAcl( bucketName, CannedAccessControlList.Private );
        try {
            userS3Client.getBucketAcl( bucketName );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "AccessDenied" ) ) {
                throw e;
            }
        }

        // 使用x-amz-grant-*方式配置桶acl,被授权人为预定义组AuthenticatedUsers，权限为FULL_CONTROL
        Grant expGrant = new Grant( GroupGrantee.AuthenticatedUsers,
                Permission.FullControl );
        PrivilegeUtils
                .setBucketAclByHeader( s3AccessKeyId, bucketName, expGrant );
        PrivilegeUtils
                .checkSetBucketAclResult( userS3Client, bucketName, expGrant );

        // 使用body配置桶acl，被授权人为桶owner，权限为FULL_CONTROL
        Grant expGrant2 = new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl );
        PrivilegeUtils
                .setBucketAclByBody( ownerS3Client, bucketName, expGrant2 );
        try {
            userS3Client.getBucketAcl( bucketName );
            Assert.fail( "expect failed but found success." );
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
