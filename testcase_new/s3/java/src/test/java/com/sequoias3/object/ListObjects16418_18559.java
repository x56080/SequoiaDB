package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-16418: To get a list by listObjectV2 within a bucket. the
 *              object has multiple versions. seqDB-18559: To get a list by
 *              listObjectV1 within a bucket.
 * @author wuyan
 * @Date 2018.11.19
 * @version 1.00
 */
public class ListObjects16418_18559 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16418";
    private String key = "aa*bb*object16418";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private int updateSize = 1024 * 10;
    private int objectNums = 200;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        updatePath =
                localPath + File.separator + "localFile_" + updateSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );
        s3Client = CommLib.buildS3Client();

        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testCreateObject() throws Exception {
        List<String> keyList = putObjects();
        listObjectsAndCheckResult( keyList );
        listObjectV1AndCheckResult( keyList );
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

    private void listObjectsAndCheckResult( List<String> keyList )
            throws IOException {
        List<String> queryKeyList = new ArrayList<>();
        ListObjectsV2Result result = s3Client.listObjectsV2( bucketName );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        Assert.assertEquals( objects.size(), objectNums );
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            String etag = os.getETag();
            long size = os.getSize();
            queryKeyList.add( key );
            // check the etag and size
            Assert.assertEquals( etag, TestTools.getMD5( updatePath ) );
            Assert.assertEquals( size, updateSize );
        }

        // check the keyName
        Collections.sort( keyList );
        Collections.sort( queryKeyList );
        Assert.assertEquals( queryKeyList, keyList );
    }

    private void listObjectV1AndCheckResult( List<String> keyList )
            throws IOException {
        List<String> queryKeyList = new ArrayList<>();
        ObjectListing result = s3Client.listObjects( bucketName );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        Assert.assertEquals( objects.size(), objectNums );
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            String etag = os.getETag();
            long size = os.getSize();
            queryKeyList.add( key );
            // check the etag and size
            Assert.assertEquals( etag, TestTools.getMD5( updatePath ) );
            Assert.assertEquals( size, updateSize );
        }

        // check the keyName
        Assert.assertEquals( queryKeyList, keyList );
    }

    private List<String> putObjects() {
        List<String> keyList = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i + TestTools.getRandomString( i );
            keyList.add( keyName );
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
            s3Client.putObject( bucketName, keyName, new File( updatePath ) );
        }

        // put deletetag object
        String deleteTagKey = "test18559";
        s3Client.deleteObject( bucketName, deleteTagKey );
        Collections.sort( keyList );
        return keyList;
    }
}
