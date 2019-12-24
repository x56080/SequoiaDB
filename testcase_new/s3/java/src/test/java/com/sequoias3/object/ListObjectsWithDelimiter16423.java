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
 * @Description seqDB-16423: To get a list of objects within a bucket.specify
 *              matching delimiter and maxkeys. test a: maxkeys is less than the
 *              matching records test b: maxkeys is greater than the matching
 *              records
 * @author wuyan
 * @Date 2018.11.23
 * @version 1.00
 */
public class ListObjectsWithDelimiter16423 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16423";
    private String key = "/aa//bb/object16423";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 100;
    private int matchObjectNums = 40;
    private File localPath = null;
    private String filePath = null;
    private String delimiter = "//";

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
    }

    @Test
    public void testListObjects() throws Exception {
        List<String> keyList = putObjects();
        int maxKeysA = 3;
        listObjectsAndCheckResult( keyList, maxKeysA );
        int maxKeysB = 20;
        listObjectsAndCheckResult( keyList, maxKeysB );
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

    private void listObjectsAndCheckResult( List<String> keyList, int maxKeys )
            throws IOException {
        List<String> queryKeyList = new ArrayList<>();
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withDelimiter( delimiter ).withMaxKeys( maxKeys );
        ListObjectsV2Result result;
        int listCount = 0;
        do {
            result = s3Client.listObjectsV2( request );
            List<String> commonPrefixes = result.getCommonPrefixes();
            // matching delimiter displays only 1 record
            if ( listCount == 0 ) {
                Assert.assertEquals( commonPrefixes.size(), 1 );
                Assert.assertEquals( commonPrefixes.get( 0 ), "/aa//" );
            }

            // objects do not match delimiter are displayed in contents,num is
            // less than 3
            List<S3ObjectSummary> objects = result.getObjectSummaries();
            if ( objects.size() > maxKeys ) {
                Assert.fail( "exceed the maximum number,  :" + objects.size() );
            }
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                queryKeyList.add( key );
            }
            listCount++;

            String continuationToken = result.getNextContinuationToken();
            request.setContinuationToken( continuationToken );
        } while ( result.isTruncated() );

        // check the query count
        int expListCount = keyList.size() / maxKeys + keyList.size() % maxKeys;
        if ( maxKeys > keyList.size() ) {
            // one query completed.
            expListCount = 1;
        }
        Assert.assertEquals( listCount, expListCount );

        // check the keyName
        Collections.sort( keyList );
        Collections.sort( queryKeyList );
        Assert.assertEquals( queryKeyList, keyList );
    }

    private List<String> putObjects() {
        List<String> noMatchKeyList = new ArrayList<>();
        int objectNums = 50;
        String keyName;
        for ( int i = 0; i < objectNums; i++ ) {
            if ( i < matchObjectNums ) {
                keyName = key + "_" + i + TestTools.getRandomString( i );
            } else {
                keyName = "object16422_" + i;
                noMatchKeyList.add( keyName );
            }
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
        }
        return noMatchKeyList;
    }
}
