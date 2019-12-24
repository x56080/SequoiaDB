package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-16355: get the object with range
 * @author wuyan
 * @Date 2018.11.13
 * @version 1.00
 */
public class GetObjectWithRange16355 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String key = "aa/bb/object16355";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;

    @DataProvider(name = "rangeProvider")
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : begin and end, fileSize is 1024 * 1024
                // range:0-0,the first bytes
                new Object[] { 0, 0 },
                // range:0,fileSize
                new Object[] { 0, 1024 * 1024 },
                // range:0,fileSize - 1
                new Object[] { 0, 1024 * 1024 - 1 },
                // medium position range: 512 * 1024, 1024 * 1024 -1
                new Object[] { 512 * 1024, 1024 * 1024 - 2 },
                // end position range: 1024 * 1024 - 1, 1024 * 1024 - 1
                new Object[] { 1024 * 1024 - 1, 1024 * 1024 - 1 }, };
    }

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

        if ( s3Client.doesObjectExist( S3TestBase.bucketName, key ) ) {
            s3Client.deleteObject( S3TestBase.bucketName, key );
        }
    }

    @Test(dataProvider = "rangeProvider")
    public void testGetObject( long start, long end ) throws Exception {
        s3Client.putObject( S3TestBase.bucketName, key, new File( filePath ) );
        getObjectAndCheckResult( S3TestBase.bucketName, start, end );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generatePageSize().length ) {
                s3Client.deleteObject( S3TestBase.bucketName, key );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void getObjectAndCheckResult( String bucketName, long start,
            long end ) throws Exception {
        GetObjectRequest request = new GetObjectRequest( bucketName, key );
        request.withRange( start, end );
        S3Object object = s3Client.getObject( request );
        S3ObjectInputStream s3is = object.getObjectContent();
        String downloadPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( s3is, downloadPath );

        // check content
        String tmpPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        TestTools.LocalFile
                .readFile( filePath, ( int ) start, ( int ) ( end - start + 1 ),
                        tmpPath );
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( tmpPath ) );
    }
}
