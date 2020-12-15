package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
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
 * @Description seqDB-19462: 桶acl配置为private，配置对象acl为public，更新桶acl
 * @Author wangkexin
 * @Date 2019.09.23
 */
public class SetBucketAcl19462 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "key19462";
    private String userName = "user19462";
    private String roleName = "normal";
    private String bucketName = "bucket19462";
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
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        // 创建用户
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
        // 使用标准acl配置桶acl为private，对象acl为public
        ownerS3Client.setBucketAcl( bucketName,
                CannedAccessControlList.Private );
        ownerS3Client.setObjectAcl( bucketName, keyName,
                CannedAccessControlList.PublicRead );
        getObjectByOtherUser();

        // 使用标准acl更新桶acl配置为public
        ownerS3Client.setBucketAcl( bucketName,
                CannedAccessControlList.PublicRead );
        Grant[] expGrant = {
                new Grant( new CanonicalGrantee( ownerId ),
                        Permission.FullControl ),
                new Grant( GroupGrantee.AllUsers, Permission.Read ) };
        PrivilegeUtils.checkSetBucketAclResult( userS3Client, bucketName,
                expGrant );
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
