package com.sequoias3.object;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
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
 * test content: 指定对象数据不存在 testlink-case: seqDB-16349
 *
 * @author wangkexin
 * @Date 2018.11.13
 * @version 1.00
 */
public class CreateObject16349 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16349";
    private String keyName = "/aa/bb/object16349.png";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        // create a path, but there is no file under the path.
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );

        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testPutObject() throws Exception {
        // put object that upload path do not exist to the bucket.
        try {
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
            Assert.fail( "exp fail but found success" );
        } catch ( SdkClientException e ) {
            Assert.assertNotEquals( e.getMessage().indexOf(
                    "Unable to calculate MD5 hash: " + filePath ), -1 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            s3Client.deleteBucket( bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
    }
}
