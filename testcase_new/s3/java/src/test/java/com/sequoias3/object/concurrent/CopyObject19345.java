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
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19345:开启版本控制，桶内并发复制相同目标对象
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19345 extends S3TestBase {
    private int runSuccessNum = 0;
    private int expRunSuccessNum = 2;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19345";
    private String srcKeyNameA = "obj19345a";
    private String srcKeyNameB = "obj19345b";
    private String dstKeyNameC = "obj19345c";
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
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        s3Client.putObject( bucketName, srcKeyNameA, new File( filePath1 ) );
        s3Client.putObject( bucketName, srcKeyNameB, new File( filePath2 ) );
    }

    // init keyNameB after copy
    @AfterMethod
    private void afterMethod() {
        String[] dstObjVers = { "0", "1", "2" };
        for ( String version : dstObjVers ) {
            s3Client.deleteVersion( bucketName, dstKeyNameC, version );
        }
    }

    @Test
    private void testCopyObject_dstObjNotExist() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec
                .addWorker( new ThreadCopyObject( srcKeyNameA, dstKeyNameC ) );
        threadExec
                .addWorker( new ThreadCopyObject( srcKeyNameB, dstKeyNameC ) );
        threadExec.run();

        // check dest object results
        String expDstObjCurVer = "1";
        String expDstObjHisVer = "0";
        try {
            // multi-thread concurrency may have result 1
            // current version
            checkObjectAttribute( expDstObjCurVer, filePath1 );
            checkObjectContent( expDstObjCurVer, filePath1 );
            // history version
            checkObjectAttribute( expDstObjHisVer, filePath2 );
            checkObjectContent( expDstObjHisVer, filePath2 );
        } catch ( AssertionError e ) {
            // or result 2
            // current version
            checkObjectAttribute( expDstObjCurVer, filePath2 );
            checkObjectContent( expDstObjCurVer, filePath2 );
            // history version
            checkObjectAttribute( expDstObjHisVer, filePath1 );
            checkObjectContent( expDstObjHisVer, filePath1 );
        }
        runSuccessNum++;
    }

    @Test
    private void testCopyObject_dstObjExist() throws Exception {
        // put object
        s3Client.putObject( bucketName, dstKeyNameC, "ready dest object" );

        // copy object
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec
                .addWorker( new ThreadCopyObject( srcKeyNameA, dstKeyNameC ) );
        threadExec
                .addWorker( new ThreadCopyObject( srcKeyNameB, dstKeyNameC ) );
        threadExec.run();

        // check dest object results
        String expDstObjCurVer = "2";
        String expDstObjHisVer = "1";
        try {
            // current version
            checkObjectAttribute( expDstObjCurVer, filePath1 );
            checkObjectContent( expDstObjCurVer, filePath1 );
            // history version
            checkObjectAttribute( expDstObjHisVer, filePath2 );
            checkObjectContent( expDstObjHisVer, filePath2 );
        } catch ( AssertionError e ) {
            // current version
            checkObjectAttribute( expDstObjCurVer, filePath2 );
            checkObjectContent( expDstObjCurVer, filePath2 );
            // history version
            checkObjectAttribute( expDstObjHisVer, filePath1 );
            checkObjectContent( expDstObjHisVer, filePath1 );
        }
        runSuccessNum++;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccessNum == expRunSuccessNum ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String expVersion, String filePath )
            throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, dstKeyNameC, expVersion );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttribute( String expVersion, String filePath )
            throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, dstKeyNameC, expVersion );
        ObjectMetadata objMetadata = s3Client.getObjectMetadata( request );
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objMetadata.getETag(), expMd5 );
        Assert.assertEquals( objMetadata.getContentLength(), fileSize );
        Assert.assertEquals( objMetadata.getVersionId(), expVersion,
                "the keyName=" + dstKeyNameC );
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
}
