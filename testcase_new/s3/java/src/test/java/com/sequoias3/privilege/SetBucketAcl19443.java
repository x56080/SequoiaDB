package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19443:配置桶acl，被授权人只有owner
 * @Author wangkexin
 * @Date 2019.09.19
 */
public class SetBucketAcl19443 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "key19443";
    private String userName = "user19443";
    private String roleName = "normal";
    private String bucketName = "bucket19443";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String ownerId;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        // 创建一个用户
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        userS3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        ownerS3Client = CommLib.buildS3Client();
        ownerId = ownerS3Client.getS3AccountOwner().getId();
        CommLib.clearBucket( ownerS3Client, bucketName );
        ownerS3Client.createBucket( new CreateBucketRequest( bucketName ) );
        ownerS3Client.putObject( bucketName, keyName, file );

    }

    @Test
    private void testSetBucketAcl() throws Exception {
        // 使用标准acl配置桶acl
        ownerS3Client
                .setBucketAcl( bucketName, CannedAccessControlList.Private );
        Grant expGrant1 = new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl );
        PrivilegeUtils.checkSetBucketAclResult( ownerS3Client, bucketName,
                expGrant1 );
        getObjectByOtherUser();

        // 使用x-amz-grant-*方式配置桶acl
        for ( Permission permission : Permission.values() ) {
            Grant expGrant2 = new Grant( new CanonicalGrantee( ownerId ),
                    permission );
            PrivilegeUtils.setBucketAclByHeader( s3AccessKeyId, bucketName,
                    expGrant2 );
            PrivilegeUtils.checkSetBucketAclResult( ownerS3Client, bucketName,
                    expGrant2 );
            getObjectByOtherUser();
        }

        // 使用body配置桶acl
        for ( Permission permission : Permission.values() ) {
            Grant expGrant3 = new Grant( new CanonicalGrantee( ownerId ),
                    permission );
            PrivilegeUtils
                    .setBucketAclByBody( ownerS3Client, bucketName, expGrant3 );
            PrivilegeUtils.checkSetBucketAclResult( ownerS3Client, bucketName,
                    expGrant3 );
            getObjectByOtherUser();
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( ownerS3Client, bucketName );
                CommLib.clearUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            ownerS3Client.shutdown();
            userS3Client.shutdown();
        }
    }

    private void getObjectByOtherUser() {
        try {
            userS3Client
                    .getObject( new GetObjectRequest( bucketName, keyName ) );
            Assert.fail(
                    "Users who are not bucket owner should fail to get object in bucket." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "AccessDenied" ) ) {
                throw e;
            }
        }
    }
}
