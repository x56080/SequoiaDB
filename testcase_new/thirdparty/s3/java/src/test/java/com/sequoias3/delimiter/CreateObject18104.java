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
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18104: create multiple objects,the object name with
 *              delimiter
 * @author wuyan
 * @Date 2019.4.11
 * @version 1.00
 */
public class CreateObject18104 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "目录aa/bb/object18104";
    private String delimiter = "%";
    private String bucketName = "bucket18104";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private File localPath = null;
    private String filePath = null;
    private AtomicInteger count = new AtomicInteger();
    private int keyNum = 1000;

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
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

    }

    @Test(invocationCount = 1000, threadPoolSize = 50)
    private void testCreateObject() throws Exception {
        int num = count.getAndIncrement();
        String subKeyName = keyName + "_" + num + "%aa%test.png";
        s3Client.putObject( bucketName, subKeyName, new File( filePath ) );
        // check the content of the create object
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, subKeyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    @Test(dependsOnMethods = "testCreateObject")
    private void testListObjectAndDeleteObject() throws Exception {
        testListObject();
        testDeleteObjectAndCheckResult();
        runSuccess = true;
    }

    private void testListObject() {
        List< String > expCommonPrefixList = new ArrayList<>();
        for ( int i = 0; i < keyNum; i++ ) {
            expCommonPrefixList.add( keyName + "_" + i + "%" );
        }
        List< String > expContextList = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                delimiter, expCommonPrefixList, expContextList );
    }

    private void testDeleteObjectAndCheckResult() throws Exception {
        for ( int i = 0; i < keyNum; i++ ) {
            String subKeyName = keyName + "_" + i + "%aa%test.png";
            s3Client.deleteObject( bucketName, subKeyName );
        }
        // check the content of the create object
        List< String > expCommprefixList = new ArrayList<>();
        List< String > expContentList = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                delimiter, expCommprefixList, expContentList );

    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
