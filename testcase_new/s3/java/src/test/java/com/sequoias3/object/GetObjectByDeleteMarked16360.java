package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
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
import java.util.List;

/**
 * @Description: seqDB-16360 ::带versionId获取删除标记的对象
 * @author fanyu
 * @Date:2018年11月12日
 * @version:1.0
 */

public class GetObjectByDeleteMarked16360 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = null;
    private String objectName = "object16360";
    private AmazonS3 s3Client = null;
    private int fileSize = 3;
    private File localPath = null;
    private List<String> filePathList = new ArrayList<String>();
    private List<PutObjectResult> objectVSList = new ArrayList<PutObjectResult>();
    private int fileNum = 3;

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
        bucketName = S3TestBase.enableVerBucketName;
        s3Client = CommLib.buildS3Client();
    }

    @Test
    private void test() throws Exception {
        // create multiple versions object in the bucket
        for ( int i = 0; i < fileNum; i++ ) {
            objectVSList.add( s3Client.putObject(
                    new PutObjectRequest( bucketName, objectName,
                            new File( filePathList.get( i ) ) ) ) );
        }
        // get the id of current version
        String versionId = objectVSList.get( fileNum - 1 ).getVersionId();
        // delete object
        s3Client.deleteObject( bucketName, objectName );
        // get the deleted version
        S3Object object = s3Client.getObject(
                new GetObjectRequest( bucketName, objectName, versionId ) );
        chectResult( object, filePathList.get( fileNum - 1 ) );

        // put new version in object
        objectVSList.add( s3Client.putObject(
                new PutObjectRequest( bucketName, objectName,
                        new File( filePathList.get( 0 ) ) ) ) );
        // get the deleted version
        S3Object object1 = s3Client.getObject(
                new GetObjectRequest( bucketName, objectName, versionId ) );
        chectResult( object1, filePathList.get( fileNum - 1 ) );

        // get the id of current version
        String currVersionId = objectVSList.get( fileNum ).getVersionId();
        // get current version object
        S3Object object2 = s3Client.getObject(
                new GetObjectRequest( bucketName, objectName, currVersionId ) );
        chectResult( object2, filePathList.get( 0 ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client, bucketName,
                        objectName );
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
