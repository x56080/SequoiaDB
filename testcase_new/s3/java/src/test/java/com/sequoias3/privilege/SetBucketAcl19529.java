package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.SetBucketAclRequest;
import com.amazonaws.services.s3.model.SetObjectAclRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19529:配置桶和对象acl，指定acl为空
 * @Author wangkexin
 * @Date 2019.09.26
 */
public class SetBucketAcl19529 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19529";
    private String keyName = "key19529";
    private AmazonS3 s3Client = null;
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

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, keyName, file );
    }

    @Test
    private void test() throws Exception {
        // 配置桶acl为空
        AccessControlList bucketAcl = new AccessControlList();
        bucketAcl.setOwner( s3Client.getS3AccountOwner() );
        s3Client.setBucketAcl(
                new SetBucketAclRequest( bucketName, bucketAcl ) );

        // 检查配置结果
        Grant[] expGrant = null;
        PrivilegeUtils
                .checkSetBucketAclResult( s3Client, bucketName, expGrant );

        // 配置对象acl为空
        AccessControlList objectAcl = new AccessControlList();
        objectAcl.setOwner( s3Client.getS3AccountOwner() );
        s3Client.setObjectAcl(
                new SetObjectAclRequest( bucketName, keyName, objectAcl ) );

        // 检查配置结果
        PrivilegeUtils.checkSetObjectAclResult( s3Client, bucketName, keyName,
                expGrant );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( bucketName, keyName );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
