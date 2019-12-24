package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-18741: Non-bucket management user lists in-progress
 *              multipart uploads.
 * @author wuyan
 * @Date 2019.08.05
 * @version 1.00
 */
public class ListMultipartUploads18741 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18741";
    private String keyName = "/aa/object18741";
    private String userNameA = "UserA18741";
    private String userNameB = "UserB18741";
    private String roleName = "normal";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;
    private File localPath = null;
    private String filePath = null;
    private File file = null;
    private int fileSize = 1024 * 1024 * 10;

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

        CommLib.clearUser( userNameA );
        CommLib.clearUser( userNameB );
        // create user A
        String[] acessKeysA = UserUtils.createUser( userNameA, roleName );
        s3ClientA = CommLib.buildS3Client( acessKeysA[ 0 ], acessKeysA[ 1 ] );
        // create user B
        String[] acessKeysB = UserUtils.createUser( userNameB, roleName );
        s3ClientB = CommLib.buildS3Client( acessKeysB[ 0 ], acessKeysB[ 1 ] );
        // create bucket
        s3ClientA.createBucket( new CreateBucketRequest( bucketName ) );

    }

    @Test
    public void uploadParts() {
        String uploadId = PartUploadUtils
                .initPartUpload( s3ClientA, bucketName, keyName );
        PartUploadUtils
                .partUpload( s3ClientA, bucketName, keyName, uploadId, file );

        // the userB is not the bucket management user
        try {
            ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                    bucketName );
            s3ClientB.listMultipartUploads( request );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied",
                    "errorCode is " + e.getErrorCode() + "  statusCode:" + e
                            .getStatusCode() );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3ClientA.shutdown();
            s3ClientB.shutdown();
        }
    }

}
