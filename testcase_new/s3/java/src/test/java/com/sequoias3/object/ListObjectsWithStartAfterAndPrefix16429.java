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
 * @Description seqDB-16429: To get a list of objects within a bucket.specify
 *              startAfter/prefix. match prefix and startAfter
 * @author wuyan
 * @Date 2018.11.27
 * @version 1.00
 */
public class ListObjectsWithStartAfterAndPrefix16429 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16429";
    private String key = "&aa&&bb&object16429.png";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private int objectNums = 1500;
    private File localPath = null;
    private String filePath = null;
    private String prefix = "&dir_1&prefix&test16429";

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
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testListObjects() throws Exception {
        List<String> keyList = putObjects();

        // test a: query returns results once
        int startAfterNoA = 1000;
        listObjectsAndCheckResult( keyList, startAfterNoA );

        // test b: query returns results multiple times
        int startAfterNoB = 100;
        listObjectsAndCheckResult( keyList, startAfterNoB );

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
            int startAfterNo ) throws IOException {
        List<String> expKeyList = new ArrayList<>( keyList );
        Collections.sort( expKeyList );
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( expKeyList.get( startAfterNo ) )
                .withPrefix( prefix );
        ListObjectsV2Result result;
        List<String> queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjectsV2( request );
            List<S3ObjectSummary> objects = result.getObjectSummaries();
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

    private List<String> putObjects() {
        List<String> matchKeyList = new ArrayList<>();
        String keyName;
        for ( int i = 0; i < objectNums; i++ ) {
            if ( i % 10 == 0 ) {
                keyName = key + "_" + i;
            } else {
                keyName = prefix + "_" + i;
                matchKeyList.add( keyName );
            }
            s3Client.putObject( bucketName, keyName, "test16429" + i );
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
        }
        return matchKeyList;
    }
}
