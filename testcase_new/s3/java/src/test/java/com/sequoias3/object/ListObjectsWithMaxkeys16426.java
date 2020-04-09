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
 * @Description seqDB-16426: To get a list of objects within a bucket.specify
 *              matching maxKeys.
 * @author wuyan
 * @Date 2018.11.23
 * @version 1.00
 */
public class ListObjectsWithMaxkeys16426 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16426";
    private String key = "%aa%%bb%object16426.png";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 100;
    private int objectNums = 200;
    private File localPath = null;
    private String filePath = null;

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
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testListObjects() throws Exception {
        List< String > keyList = putObjects();
        // test a: maxkeys < objectNums
        int maxKeysA = 5;
        listObjectsAndCheckResult( keyList, maxKeysA );

        // test b: maxKeys > objectNums
        int maxKeysB = objectNums + 1;
        listObjectsAndCheckResult( keyList, maxKeysB );

        // test c: maxKeys = objectNums
        int maxKeysC = objectNums;
        listObjectsAndCheckResult( keyList, maxKeysC );

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
            int maxKeys ) throws IOException {
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withMaxKeys( maxKeys );
        ListObjectsV2Result result;
        List< String > queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjectsV2( request );
            List< S3ObjectSummary > objects = result.getObjectSummaries();
            int oneQueryKeyNums = 0;
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                // oneQueryKeyList.add(key);
                queryKeyList.add( key );
                oneQueryKeyNums++;
            }

            if ( maxKeys < objectNums ) {
                Assert.assertEquals( oneQueryKeyNums, maxKeys );
            } else {
                Assert.assertEquals( oneQueryKeyNums, objectNums );
            }
            String continuationToken = result.getNextContinuationToken();
            request.setContinuationToken( continuationToken );
        } while ( result.isTruncated() );

        // check the keyName
        Collections.sort( keyList );
        Collections.sort( queryKeyList );
        Assert.assertEquals( queryKeyList, keyList );
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
