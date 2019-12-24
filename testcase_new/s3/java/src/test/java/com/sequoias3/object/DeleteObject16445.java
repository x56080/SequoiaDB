package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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
 * test content: Suspended bucket versioning , delete object on the bucket
 * testlink-case: seqDB-16445
 *
 * @author wuyan
 * @Date 2018.11.22
 * @version 1.00
 */
public class DeleteObject16445 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16445";
    private String key = "$$aa$%maa$bb*中文$object16445";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 500;
    private File localPath = null;
    private String filePath = null;

    @SuppressWarnings("deprecation")
    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        if ( s3Client.doesBucketExist( bucketName ) ) {
            ObjectUtils.deleteObjectAllVersions( s3Client, bucketName, key );
            s3Client.deleteBucket( bucketName );
        }

        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
    }

    @Test
    public void testDeleteObject() {
        s3Client.putObject( bucketName, key, new File( filePath ) );
        s3Client.deleteObject( bucketName, key );
        checkDeleteObjectResult( bucketName, key );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils
                        .deleteObjectAllVersions( s3Client, bucketName, key );
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDeleteObjectResult( String bucketName, String key ) {
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertFalse( isExistObject, "the object should not exist!" );
        try {
            s3Client.getObject( bucketName, key );
            Assert.fail( "get not exist key must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
    }
}
