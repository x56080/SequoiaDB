package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18106: create object,the object name include old
 *              delimiter.
 * @author wuyan
 * @Date 2019.04.12
 * @version 1.00
 */
public class CreateObject18106 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18106";
    private String keyName = "/aa/object18106.txt";
    private String newDelimiter = "%";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws Exception {
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
        DelimiterUtils.putBucketDelimiter( bucketName, newDelimiter );
    }

    @Test
    public void testCreateObject() throws Exception {
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        // check the content of the create object
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        List< String > matchDelimiterKeyList = new ArrayList<>();
        List< String > expContentList = new ArrayList<>();
        expContentList.add( keyName );
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                newDelimiter, matchDelimiterKeyList, expContentList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( bucketName, keyName );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
