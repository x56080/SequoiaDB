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
 * @Description seqDB-19459: 桶acl未配置，对象acl配置为private，更新对象acl
 * @Author wangkexin
 * @Date 2019.09.19
 */
public class SetObjectAcl19459 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user19459";
    private String roleName = "normal";
    private String bucketName = "bucket19459";
    private String keyName = "key19459";
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
    private void testSetObjectAcl() throws Exception {
        // 使用标准acl配置对象acl为private
        ownerS3Client.setObjectAcl( bucketName, keyName,
                CannedAccessControlList.Private );
        try {
            userS3Client.getObjectAcl( bucketName, keyName );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "AccessDenied" ) ) {
                throw e;
            }
        }

        // 使用x-amz-grant-*方式配置对象acl,被授权人为预定义组AllUsers，权限为WRITE
        Grant expGrant = new Grant( GroupGrantee.AllUsers, Permission.Write );
        PrivilegeUtils.setObjectAclByHeader( s3AccessKeyId, bucketName, keyName,
                expGrant );
        PrivilegeUtils
                .checkSetObjectAclResult( userS3Client, bucketName, keyName,
                        expGrant );

        // 使用body配置对象acl，被授权人为桶owner，权限为FULL_CONTROL
        Grant expGrant2 = new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl );
        PrivilegeUtils.setObjectAclByBody( ownerS3Client, bucketName, keyName,
                expGrant2 );
        PrivilegeUtils
                .checkSetObjectAclResult( userS3Client, bucketName, keyName,
                        expGrant2 );

        // 使用桶owner用户查看桶acl为默认配置
        Grant expDefaultBucketAcl = new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl );
        PrivilegeUtils
                .checkSetObjectAclResult( ownerS3Client, bucketName, keyName,
                        expDefaultBucketAcl );
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
}
