package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
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
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Random;

/**
 * @Description: seqDB-16382 :: 指定ifNoneMatch和ifModifiedSince条件获取对象
 * @author fanyu
 * @Date:2018年11月14日
 * @version:1.0
 */

public class GetObjectByNoEtagAndModifiedSince16382 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16382";
    private String objectName = "object16382";
    private AmazonS3 s3Client = null;
    private int fileSize = 2;
    private File localPath = null;
    private List<String> filePathList = new ArrayList<String>();
    private List<PutObjectResult> objectVSList = new ArrayList<PutObjectResult>();
    private int fileNum = 2;
    private Calendar cal = Calendar.getInstance();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        String filePath = null;
        for ( int i = 0; i < fileNum; i++ ) {
            filePath =
                    localPath + File.separator + "localFile_" + ( fileSize + i )
                            + ".txt";
            TestTools.LocalFile.createFile( filePath, fileSize + i );
            filePathList.add( filePath );
        }
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void test() throws Exception {
        // create multiple versions object in the bucket
        for ( int i = 0; i < fileNum; i++ ) {
            objectVSList.add( s3Client.putObject(
                    new PutObjectRequest( bucketName, objectName,
                            new File( filePathList.get( i ) ) ) ) );
        }

        // get history eTag
        Random random = new Random();
        int histIndex = random.nextInt( fileNum - 1 );
        String histETag = objectVSList.get( histIndex ).getETag();

        // get object by no eTag and modified
        // the object has been modified since now-one_month
        cal.set( Calendar.MONTH, cal.get( Calendar.MONTH ) - 1 );
        S3Object currObject = s3Client.getObject(
                new GetObjectRequest( bucketName, objectName )
                        .withNonmatchingETagConstraint( histETag )
                        .withModifiedSinceConstraint( cal.getTime() ) );
        // check the eTag and the content of object
        String currPath = filePathList.get( fileNum - 1 );
        chectResult( currObject, currPath );
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
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void chectResult( S3Object object, String filePath )
            throws Exception {
        Assert.assertEquals( object.getObjectMetadata().getETag(),
                TestTools.getMD5( filePath ) );
        S3ObjectInputStream s3InputStream = null;
        try {
            s3InputStream = object.getObjectContent();
            String downloadPath = TestTools.LocalFile
                    .initDownloadPath( localPath, TestTools.getMethodName(),
                            Thread.currentThread().getId() );
            ObjectUtils.inputStream2File( s3InputStream, downloadPath );
            Assert.assertEquals( TestTools.getMD5( downloadPath ),
                    TestTools.getMD5( filePath ) );
        } finally {
            if ( s3InputStream != null ) {
                s3InputStream.close();
            }
        }
    }
}
