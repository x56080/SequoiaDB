package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18118: To get a list of objects within a bucket.specify
 *              matching delimiter,all objects match delimiter.
 * @author wuyan
 * @Date 2019.04.23
 * @version 1.00
 */
public class ListObjects18118 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18118";
    private String delimiter = "test";
    private AmazonS3 s3Client = null;
    private String[] keyList = { "dir1/test1_18118.png",
            "dir1/dir2/dir3/test2_18118", "dir1/test3_18118",
            "dir1/dir2/aa/test4_18118", "test5_18118",
            "dir1/dir2/aa/test6_18118", "test/test18118.png" };

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testCreateObject() throws Exception {
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        putObjects();
        listObjectsAndCheckResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( int i = 0; i < keyList.length; i++ ) {
                    String keyName = keyList[ i ];
                    s3Client.deleteObject( bucketName, keyName );
                }
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void putObjects() {
        for ( int i = 0; i < keyList.length; i++ ) {
            String subKeyName = keyList[ i ];
            s3Client.putObject( bucketName, subKeyName,
                    "testcontext18118_" + i );
        }
    }

    private void listObjectsAndCheckResult() {
        List< String > matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "dir1/test" );
        matchPrefixList.add( "dir1/dir2/dir3/test" );
        matchPrefixList.add( "dir1/dir2/aa/test" );
        matchPrefixList.add( "test" );
        List< String > matchContentsList = new ArrayList<>();

        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                delimiter, matchPrefixList, matchContentsList );
    }
}
