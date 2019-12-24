package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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
 * @Description seqDB-16442/16443: test 16442:delete object test 16443:delete
 *              objects that do not exist
 * @author wuyan
 * @Date 2018.11.21
 * @version 1.00
 */
public class DeleteObject16442_16443 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "&&aa&%maa&bb*中文&object16442";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 300;
    private File localPath = null;
    private String filePath = null;

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
    }

    @Test
    public void testDeleteObject() {
        // test 16442:delete object
        s3Client.putObject( S3TestBase.bucketName, key, new File( filePath ) );
        s3Client.deleteObject( S3TestBase.bucketName, key );
        checkDeleteObjectResult( S3TestBase.bucketName, key );

        // test 16443:delete object that do not exist
        s3Client.deleteObject( S3TestBase.bucketName, key );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                if ( s3Client.doesObjectExist( S3TestBase.bucketName, key ) ) {
                    s3Client.deleteObject( S3TestBase.bucketName, key );
                }
                TestTools.LocalFile.removeFile( localPath );
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
