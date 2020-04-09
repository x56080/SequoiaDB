package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
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
 * @Description seqDB-19453:桶开启版本控制，配置对象acl，被授权人只有owner
 * @Author wangkexin
 * @Date 2019.09.23
 */
public class SetObjectAcl19453 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "key19453";
    private String userName = "user19453";
    private String roleName = "normal";
    private String bucketName = "bucket19453";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;
    private long oldFileSize = 100 * 1024;
    private long newFileSize = 200 * 1024;
    private File localPath = null;
    private File oldFile = null;
    private File newFile = null;
    private String oldFilePath = null;
    private String newFilePath = null;
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
        CommLib.setBucketVersioning( ownerS3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        putObjectBeforeTest();
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        CannedAccessControlList[] aclArray = { CannedAccessControlList.Private,
                CannedAccessControlList.AwsExecRead,
                CannedAccessControlList.BucketOwnerFullControl,
                CannedAccessControlList.BucketOwnerRead };
        // 使用标准acl配置对象acl
        for ( CannedAccessControlList acl : aclArray ) {
            ownerS3Client.setObjectAcl( bucketName, keyName, acl );
            Grant expGrant1 = new Grant( new CanonicalGrantee( ownerId ),
                    Permission.FullControl );
            PrivilegeUtils.checkSetObjectAclResult( ownerS3Client, bucketName,
                    keyName, expGrant1 );
            getObjectByOtherUser();
        }

        // 使用x-amz-grant-*方式配置对象acl
        for ( Permission permission : Permission.values() ) {
            Grant expGrant2 = new Grant( new CanonicalGrantee( ownerId ),
                    permission );
            PrivilegeUtils.setObjectAclByHeader( s3AccessKeyId, bucketName,
                    keyName, expGrant2 );
            PrivilegeUtils.checkSetObjectAclResult( ownerS3Client, bucketName,
                    keyName, expGrant2 );
            getObjectByOtherUser();
        }

        // 使用body配置对象acl
        for ( Permission permission : Permission.values() ) {
            Grant expGrant3 = new Grant( new CanonicalGrantee( ownerId ),
                    permission );
            PrivilegeUtils.setObjectAclByBody( ownerS3Client, bucketName,
                    keyName, expGrant3 );
            PrivilegeUtils.checkSetObjectAclResult( ownerS3Client, bucketName,
                    keyName, expGrant3 );
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
                    "Users who are not object owner should fail to get object in bucket." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "AccessDenied" ) ) {
                throw e;
            }
        }
    }

    private void putObjectBeforeTest() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        oldFilePath = localPath + File.separator + "localFile_" + oldFileSize
                + ".txt";
        newFilePath = localPath + File.separator + "localFile_" + newFileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( oldFilePath, oldFileSize );
        TestTools.LocalFile.createFile( newFilePath, newFileSize );
        oldFile = new File( oldFilePath );
        newFile = new File( newFilePath );

        ownerS3Client.putObject( bucketName, keyName, oldFile );
        ownerS3Client.putObject( bucketName, keyName, newFile );
    }
}
