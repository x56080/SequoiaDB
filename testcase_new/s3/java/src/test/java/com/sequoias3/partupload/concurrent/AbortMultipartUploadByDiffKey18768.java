package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18768: the key upload multiple parts and
 *              AbortMultipartUpload concurrently by the different keys.
 * @author wuyan
 * @Date 2019.08.08
 * @version 1.00
 */
public class AbortMultipartUploadByDiffKey18768 extends S3TestBase {
    private boolean runSuccess = false;
    private String baseKeyName = "/aa/object18768";
    private int keyNum = 20;
    private List<String> keyNames = new ArrayList<>();
    private String bucketName = "bucket18768";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 28;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void abortMultipartUpload() throws Exception {
        File file = new File( filePath );
        ThreadExecutor threadExec = new ThreadExecutor();
        for ( int i = 0; i < keyNum; i++ ) {
            String keyName = baseKeyName + "/" + i + "_.txt";
            threadExec.addWorker( new AbortMultipartUpload( file, keyName ) );
            keyNames.add( keyName );
        }
        threadExec.run();
        checkResult();

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

    private void checkResult() {
        for ( int i = 0; i < keyNum; i++ ) {
            String keyName = keyNames.get( i );
            // get key is not exist.
            try {
                s3Client.getObject( bucketName, keyName );
                Assert.fail( "get not exist key must be fail !" );
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
            }
        }

    }

    private class AbortMultipartUpload extends ResultStore {
        private String uploadId;
        private String keyName;
        private File file = null;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        private AbortMultipartUpload( File file, String keyName ) {
            this.file = file;
            this.keyName = keyName;
        }

        @ExecuteOrder(step = 1)
        private void initPartUpload() {
            uploadId = PartUploadUtils
                    .initPartUpload( s3Client1, bucketName, keyName );
        }

        @ExecuteOrder(step = 2)
        private void partUpload() {
            PartUploadUtils
                    .partUpload( s3Client1, bucketName, keyName, uploadId,
                            file );
        }

        @ExecuteOrder(step = 3)
        private void completeMultipartUpload() throws IOException {
            try {
                AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                        bucketName, keyName, uploadId );
                s3Client.abortMultipartUpload( request );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }
}
