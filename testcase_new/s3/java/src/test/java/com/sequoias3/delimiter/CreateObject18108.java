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
 * @Description seqDB-18108: create object,the object name match directory
 *              already exists.
 * @author wuyan
 * @Date 2019.04.12
 * @version 1.00
 */
public class CreateObject18108 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18108";
    private String keyName1 = "aa/bb/object18108.txt";
    private String keyName2 = "aa/bb/cc/dd/aa/bb/object18108.txt";
    private String defalutDelimiter = "/";
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
    }

    @Test
    public void testCreateObject() throws Exception {
        s3Client.putObject( bucketName, keyName1, new File( filePath ) );
        // check the content of the create object
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName1 );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        s3Client.putObject( bucketName, keyName2, new File( filePath ) );
        List< String > matchDelimiterKeyList = new ArrayList<>();
        matchDelimiterKeyList.add( "aa/" );
        List< String > expContextList = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                defalutDelimiter, matchDelimiterKeyList, expContextList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( bucketName, keyName1 );
                s3Client.deleteObject( bucketName, keyName2 );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
