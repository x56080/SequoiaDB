package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
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
import java.util.List;

/**
 * @Description seqDB-16427: To get a list of objects within a bucket.specify
 *              startAfter/prefix. match startAfter, not match prefix
 * @author wuyan
 * @Date 2018.11.24
 * @version 1.00
 */
public class ListObjectsWithStartAfterAndPrefix16427 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16427";
    private String key = "*aa**bb*object16427.png";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 20;
    private int objectNums = 10;
    private File localPath = null;
    private String filePath = null;

    @SuppressWarnings("deprecation")
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

        if ( s3Client.doesBucketExist( bucketName ) ) {
            CommLib.clearBucket( s3Client, bucketName );
        }

        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testListObjects() throws Exception {
        List<String> keyList = putObjects();

        int startAfterNo = 0;
        String prefix = "pre16427";
        listObjectsAndCheckResult( keyList, startAfterNo, prefix );
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

    private void listObjectsAndCheckResult( List<String> keyList,
            int startAfterNo, String prefix ) {
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( keyList.get( startAfterNo ) )
                .withPrefix( prefix );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        List<String> commonPrefixes = result.getCommonPrefixes();
        // misMatchObject, the list size is 0
        Assert.assertEquals( objects.size(), 0 );
        Assert.assertEquals( commonPrefixes.size(), 0 );
    }

    private List<String> putObjects() {
        List<String> keyList = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i;
            keyList.add( keyName );
            s3Client.putObject( bucketName, keyName, "test16427" + i );
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
        }
        return keyList;
    }
}
