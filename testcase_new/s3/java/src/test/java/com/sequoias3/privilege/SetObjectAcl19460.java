package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19460:桶acl和对象acl配置为private，更新对象acl
 * @Author wangkexin
 * @Date 2019.09.23
 */
public class SetObjectAcl19460 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user19460";
    private String roleName = "normal";
    private String bucketName = "bucket19460";
    private String keyName = "key19460";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

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
        CommLib.clearBucket( ownerS3Client, bucketName );
        ownerS3Client.createBucket( new CreateBucketRequest( bucketName ) );
        ownerS3Client.putObject( bucketName, keyName, file );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        // 使用标准acl配置桶acl为private，对象acl为private
        ownerS3Client
                .setBucketAcl( bucketName, CannedAccessControlList.Private );
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

        // 使用标准acl配置桶acl为public-read
        ownerS3Client.setObjectAcl( bucketName, keyName,
                CannedAccessControlList.PublicRead );
        try {
            userS3Client.getObjectAcl( bucketName, keyName );
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
