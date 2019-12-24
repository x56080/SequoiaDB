package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19314:不同桶复制对象，指定目标桶不开启版本控制，且指定目标对象已存在
 * @author wuyan
 * @Date 2019.09.18
 * @version 1.00
 */
public class CopyObject19314 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "/srcObject19314";
    private String destKeyName = "/dest/object19314";
    private String srcBucketName = "srcbucket19314";
    private String destBucketName = "destbucket19314";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 10;
    private int copyFileSize = 1024 * 1024 * 30;
    private File localPath = null;
    private String filePath = null;
    private String copyFilePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        copyFilePath = localPath + File.separator + "localFile_" + copyFileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( copyFilePath, copyFileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, destBucketName );
        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( destBucketName );

        s3Client.putObject( srcBucketName, srcKeyName,
                new File( copyFilePath ) );
        s3Client.putObject( destBucketName, destKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        s3Client.copyObject( srcBucketName, srcKeyName, destBucketName,
                destKeyName );
        checkObjectContent( destBucketName, destKeyName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, destBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( copyFilePath ) );
    }
}
