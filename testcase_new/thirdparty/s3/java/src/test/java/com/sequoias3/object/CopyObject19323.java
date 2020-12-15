package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
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
 * @Description seqDB-19323:复制对象指定源对象内容为空
 * @author wuyan
 * @Date 2019.09.18
 * @version 1.00
 */
public class CopyObject19323 extends S3TestBase {

    private boolean runSuccess = false;
    private String srcKeyName = "/srcObject19323";
    private String destKeyName = "/dest/object19323";
    private String bucketName = "bucket19323";
    private AmazonS3 s3Client = null;
    private int fileSize = 0;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        s3Client.copyObject( bucketName, srcKeyName, bucketName, destKeyName );

        checkObjectAttributeInfo( bucketName, destKeyName );
        checkObjectContent( bucketName, destKeyName );

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

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttributeInfo( String bucketName, String keyName )
            throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );

        Assert.assertEquals( result.getETag(), TestTools.getMD5( filePath ) );
        Assert.assertEquals( result.getContentLength(), fileSize );
    }
}
