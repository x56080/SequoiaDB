package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19463: 桶开启版本控制，配置对象acl，其中桶或对象不存在
 * @Author wangkexin
 * @Date 2019.09.23
 */
public class SetObjectAcl19463 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19463";
    private String non_existBucket = "nonexistentbucket19463";
    private String keyName = "key19463";
    private String non_existObject = "nonexistentobject19463";
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private AmazonS3 s3Client = null;

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
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        s3Client.putObject( bucketName, keyName, file );
        s3Client.putObject( bucketName, keyName, "testContent19463" );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        // 使用标准acl配置对象acl，其中指定桶不存在
        try {
            s3Client.setObjectAcl( non_existBucket, keyName,
                    CannedAccessControlList.PublicReadWrite );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "NoSuchBucket" ) ) {
                throw e;
            }
        }

        // 使用标准acl配置对象acl，其中指定对象不存在（指定versionId）
        try {
            s3Client.setObjectAcl( bucketName, non_existObject, "0",
                    CannedAccessControlList.PublicReadWrite );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "NoSuchVersion" ) ) {
                throw e;
            }
        }

        // 使用标准acl配置对象acl，其中指定对象不存在（指定versionId）
        try {
            s3Client.setObjectAcl( bucketName, non_existObject,
                    CannedAccessControlList.PublicReadWrite );
            Assert.fail( "expect failed but found success." );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "NoSuchKey" ) ) {
                throw e;
            }
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
