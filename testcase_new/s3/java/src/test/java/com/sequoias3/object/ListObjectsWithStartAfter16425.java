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
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-16425: To get a list of objects within a bucket.specify
 *              matching startAfter.
 * @author wuyan
 * @Date 2018.11.23
 * @version 1.00
 */
public class ListObjectsWithStartAfter16425 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16425";
    private String key = "/aa//bb/object16425.png";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 100;
    private int objectNums = 200;
    private File localPath = null;
    private String filePath = null;

    @SuppressWarnings("deprecation")
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

        if ( s3Client.doesBucketExist( bucketName ) ) {
            CommLib.clearBucket( s3Client, bucketName );
        }

        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testListObjects() throws Exception {
        List< String > keyList = putObjects();
        // test a: starting in the middle
        int startAfterNoA = objectNums / 2;
        listObjectsAndCheckResult( keyList, startAfterNoA );

        // test b: starting with the first record
        int startAfterNoB = 0;
        listObjectsAndCheckResult( keyList, startAfterNoB );

        // test c:starting with the last one
        int startAfterNoC = objectNums - 1;
        listObjectsAndCheckResult( keyList, startAfterNoC );

        // test d: show only the last object
        int startAfterNoD = objectNums - 2;
        listObjectsAndCheckResult( keyList, startAfterNoD );

        // test e: mismatch the object
        String startAfter = "test16424.txt";
        misMatchObject( startAfter );
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

    private void listObjectsAndCheckResult( List< String > keyList,
            int startAfterNo ) throws IOException {
        List< String > expKeyList = new ArrayList<>( keyList );
        Collections.sort( expKeyList );
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( expKeyList.get( startAfterNo ) );
        ListObjectsV2Result result;
        List< String > queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjectsV2( request );
            List< S3ObjectSummary > objects = result.getObjectSummaries();
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                queryKeyList.add( key );
            }
            String continuationToken = result.getNextContinuationToken();
            request.setContinuationToken( continuationToken );
        } while ( result.isTruncated() );

        // check the keyName
        Collections.sort( queryKeyList );
        for ( int i = 0; i < startAfterNo + 1; i++ ) {
            expKeyList.remove( 0 );
        }

        Assert.assertEquals( queryKeyList, expKeyList );
    }

    private void misMatchObject( String startAfter ) {
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( startAfter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< S3ObjectSummary > objects = result.getObjectSummaries();
        // misMatchObject, the list size is 0
        Assert.assertEquals( objects.size(), 0 );
    }

    private List< String > putObjects() {
        List< String > keyList = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i;
            keyList.add( keyName );
            s3Client.putObject( bucketName, keyName, "test16424" + i );
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
        }
        return keyList;
    }
}
