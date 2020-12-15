package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-16502: enabling bucket versioning ,concurrent delete the
 *              same object on the bucket
 * @author wuyan
 * @Date 2019.1.9
 * @version 1.00
 */
public class DeleteSameObjectWithVersionId16502 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "aa%maa%bb*中文/object16502";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 20;
    private int updateSize = 1024 * 1024 * 2;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
    private String deleteVersionId = "1";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );

        s3Client = CommLib.buildS3Client();
        ObjectUtils.deleteObjectAllVersions( s3Client,
                S3TestBase.enableVerBucketName, key );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( filePath ) );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( updatePath ) );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( updatePath ) );
    }

    @Test
    public void testDeleteObject() throws Exception {
        DeleteObjectThread deleteObjectThread = new DeleteObjectThread();
        deleteObjectThread.start( 50 );
        Assert.assertTrue( deleteObjectThread.isSuccess(),
                deleteObjectThread.getErrorMsg() );
        checkDeleteObjectResult( S3TestBase.enableVerBucketName, key );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client,
                        S3TestBase.enableVerBucketName, key );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDeleteObjectResult( String bucketName, String key )
            throws Exception {
        // current version object exist
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertTrue( isExistObject, "the object should be exist!" );

        // the delete object is not exist.
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, key, deleteVersionId );
        try {
            s3Client.getObjectMetadata( request );
            Assert.fail( "head object must be fail!" );
        } catch ( AmazonS3Exception e ) {
            // 404 Not Found
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        // the oldest version updated to the latest history version
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key, "0" );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private class DeleteObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.deleteVersion( S3TestBase.enableVerBucketName, key,
                        deleteVersionId );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
