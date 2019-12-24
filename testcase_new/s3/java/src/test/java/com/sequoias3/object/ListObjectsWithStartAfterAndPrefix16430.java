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
 * @Description seqDB-16430: To get a list of objects within a bucket.specify
 *              startAfter/prefix/maxkeys. match prefix , startAfter and
 *              maxkeys.
 * @author wuyan
 * @Date 2018.11.27
 * @version 1.00
 */
public class ListObjectsWithStartAfterAndPrefix16430 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16430";
    private String key = "!aa!!bb!object16430.png";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private int objectNums = 100;
    private File localPath = null;
    private String filePath = null;
    private String prefix = "!dir_1!prefix!test16430";
    ;

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

        // test a: query returns results once
        int startAfterNoA = 30;
        int maxKeysA = 51;
        listObjectsAndCheckResult( keyList, startAfterNoA, maxKeysA );

        // test b: query returns results multiple times, each return nums is
        // maxkeys
        int startAfterNoB = 4;
        int maxKeysB = 5;
        listObjectsAndCheckResult( keyList, startAfterNoB, maxKeysB );

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
            int startAfterNo, int maxKeys ) throws IOException {
        List<String> expKeyList = new ArrayList<>( keyList );
        Collections.sort( expKeyList );
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( expKeyList.get( startAfterNo ) )
                .withPrefix( prefix ).withMaxKeys( maxKeys );
        ListObjectsV2Result result;
        List<String> queryKeyList = new ArrayList<>();
        do {
            List<String> oneQueryKeyList = new ArrayList<>();
            result = s3Client.listObjectsV2( request );
            List<S3ObjectSummary> objects = result.getObjectSummaries();
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                oneQueryKeyList.add( key );
                queryKeyList.add( key );
            }

            int expMatchObjectNums = keyList.size() - startAfterNo - 1;
            if ( maxKeys < keyList.size() ) {
                Assert.assertEquals( oneQueryKeyList.size(), maxKeys );
            } else {
                Assert.assertEquals( oneQueryKeyList.size(),
                        expMatchObjectNums );
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
            if ( i % 2 == 0 ) {
                keyName = key + "_" + i;
            } else {
                keyName = prefix + "_" + i;
                matchKeyList.add( keyName );
            }
            s3Client.putObject( bucketName, keyName, "test16430" + i );
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
        }
        return matchKeyList;
    }
}
