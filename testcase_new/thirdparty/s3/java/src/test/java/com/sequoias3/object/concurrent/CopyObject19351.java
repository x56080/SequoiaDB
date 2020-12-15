package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
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
 * @Description seqDB-19351:并发增加和复制相同目标对象
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19351 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19351";
    private String srcKeyName = "srcObj19351";
    private String dstKeyName = "dstObj19351";
    private int fileSize = 5 * 1024 * 1024;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath1 = localPath + File.separator + "localFile_" + fileSize
                + "_1.txt";
        filePath2 = localPath + File.separator + "localFile_" + fileSize
                + "_2.txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize );
        TestTools.LocalFile.createFile( filePath2, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );

        s3Client.putObject( bucketName, srcKeyName, new File( filePath1 ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadCopyObject( srcKeyName, dstKeyName ) );
        threadExec.addWorker( new ThreadPutObject( dstKeyName ) );
        threadExec.run();

        try {
            // multi-thread concurrency may have result 1
            checkObjectAttribute( dstKeyName, filePath1 );
            checkObjectContent( dstKeyName, filePath1 );
        } catch ( AssertionError e ) {
            // or result 2
            checkObjectAttribute( dstKeyName, filePath2 );
            checkObjectContent( dstKeyName, filePath2 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String keyName, String filePath )
            throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttribute( String keyName, String filePath )
            throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata objMetadata = s3Client.getObjectMetadata( request );
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objMetadata.getETag(), expMd5 );
        Assert.assertEquals( objMetadata.getContentLength(), fileSize );
        Assert.assertEquals( objMetadata.getVersionId(), "null",
                "the keyName=" + keyName );
    }

    private class ThreadCopyObject {
        private String srcKeyName;
        private String dstKeyName;

        private ThreadCopyObject( String srcKeyName, String dstKeyName ) {
            this.srcKeyName = srcKeyName;
            this.dstKeyName = dstKeyName;
        }

        @ExecuteOrder(step = 1)
        private void copyObject() {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                CopyObjectRequest request = new CopyObjectRequest( bucketName,
                        srcKeyName, bucketName, dstKeyName );
                s3.copyObject( request );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadPutObject {
        private String keyName;

        private ThreadPutObject( String keyName ) {
            this.keyName = keyName;
        }

        @ExecuteOrder(step = 1)
        private void copyObject() {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                s3.putObject( bucketName, keyName, new File( filePath2 ) );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
