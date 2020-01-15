package com.sequoias3.partupload;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * @Description seqDB-18726:非桶管理用户终止分段上传
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class AbortMultipartUpload18726 extends S3TestBase {
    private boolean runSuccess = false;
    private String userA = "user18726_a";
    private String userB = "user18726_b";
    private AmazonS3 s3ClientA;
    private AmazonS3 s3ClientB;
    private String bucketName = "bucket18726";
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 5 * 1024 * 1024;
    private int maxPartNumber = 5;
    private String key = "/aa/bb/obj18726";

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();

        CommLib.clearUser( userA );
        CommLib.clearUser( userB );
        String[] accessKeysA = UserUtils
                .createUser( userA, UserCommDefind.normal );
        String[] accessKeysB = UserUtils
                .createUser( userB, UserCommDefind.normal );
        s3ClientA = CommLib.buildS3Client( accessKeysA[ 0 ], accessKeysA[ 1 ] );
        s3ClientB = CommLib.buildS3Client( accessKeysB[ 0 ], accessKeysB[ 1 ] );

        s3ClientA.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void test() throws Exception {
        // userA upload part
        String uploadId = PartUploadUtils
                .initPartUpload( s3ClientA, bucketName, key );
        List<PartETag> partETags = PartUploadUtils
                .partUpload( s3ClientA, bucketName, key, uploadId, file,
                        fileSize / maxPartNumber );

        // userB abort upload
        try {
            s3ClientB.abortMultipartUpload(
                    new AbortMultipartUploadRequest( bucketName, key,
                            uploadId ) );
        } catch ( AmazonServiceException e ) {
            if ( !e.getErrorCode().equals( "AccessDenied" ) ) {
                Assert.fail( e.getMessage() );
            }
        }
        PartUploadUtils
                .listPartsAndCheckPartNumbers( s3ClientA, bucketName, key,
                        partETags, uploadId );

        // userA abort again
        s3ClientA.abortMultipartUpload(
                new AbortMultipartUploadRequest( bucketName, key, uploadId ) );
        PartUploadUtils
                .checkAbortMultipartUploadResult( s3ClientA, bucketName, key,
                        uploadId );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3ClientA.deleteBucket( bucketName );
                CommLib.clearUser( userA );
                CommLib.clearUser( userB );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3ClientA.shutdown();
            s3ClientB.shutdown();
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