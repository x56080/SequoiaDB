package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-18717:初始化分段上传后，终止分段上传
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class AbortMultipartUpload18717 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client;
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 5 * 1024 * 1024;
    private int maxPartNumber = 5;
    private String key = "/aa/bb/obj18717";

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
    }

    @Test
    private void test() throws Exception {
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, key );
        PartUploadUtils
                .partUpload( s3Client, S3TestBase.bucketName, key, uploadId,
                        file, fileSize / maxPartNumber );
        s3Client.abortMultipartUpload(
                new AbortMultipartUploadRequest( bucketName, key, uploadId ) );
        PartUploadUtils
                .checkAbortMultipartUploadResult( s3Client, bucketName, key,
                        uploadId );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void initFile() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );
    }
}