package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-18345: concurrent create object and delete bucket.the
 *              object name include delimiter
 *
 * @author wuyan
 * @Date 2019.5.21
 * @version 1.00
 */

public class CreateObjectAndDeleteBucket18345 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18345";
    private String keyName = "dir1/test18345";
    private String delimiter = "?";
    private int objectNum = 10;
    private AmazonS3 s3Client = null;
    private List<String> keyList = new ArrayList<>();

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @SuppressWarnings("deprecation")
    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        for ( int i = 0; i < objectNum; i++ ) {
            String subKeyName = keyName + "_" + i + delimiter + "test.png";
            keyList.add( subKeyName );
            threadExec.addWorker( new CreateObject( subKeyName ) );
        }
        DeleteBucket deleteBucket = new DeleteBucket();
        threadExec.addWorker( deleteBucket );
        threadExec.run();

        int errorDeleteBucketCode = deleteBucket.getRetCode();
        if ( errorDeleteBucketCode == 0 ) {
            // delete bucket success, create object fail.
            Assert.assertFalse( s3Client.doesBucketExist( bucketName ) );
            for ( String keyName : keyList ) {
                Assert.assertFalse(
                        s3Client.doesObjectExist( bucketName, keyName ) );
            }
        } else {
            // delete bucekt fail, create object success, the
            // errorCode:409(BucketNotEmpty).
            Assert.assertEquals( errorDeleteBucketCode, 409 );
            Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );
            listObjectsAndCheckResult( keyList );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void listObjectsAndCheckResult( List<String> keyList )
            throws IOException {
        List<String> queryKeyList = new ArrayList<>();
        ListObjectsV2Result result = s3Client.listObjectsV2( bucketName );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }

        // check the keyName
        Collections.sort( keyList );
        Assert.assertEquals( queryKeyList, keyList,
                "queryKeyList = " + queryKeyList + ", keyList = " + keyList );
    }

    private class CreateObject {
        private String keyName;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        private CreateObject( String keyName ) {
            this.keyName = keyName;
        }

        @ExecuteOrder(step = 1)
        private void createObject() {
            try {
                String content = keyName + "_testcontent";
                s3Client1.putObject( bucketName, keyName, content );
            } catch ( AmazonS3Exception e ) {
                int errCode = e.getStatusCode();
                // 404:NoSuchBucket
                if ( errCode != 404 ) {
                    Assert.fail(
                            "create object fail! errCode:" + errCode + "," + e
                                    .getErrorMessage() );
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private class DeleteBucket extends ResultStore {
        private AmazonS3 s3Client2 = CommLib.buildS3Client();

        @ExecuteOrder(step = 1)
        private void deleteBucket() throws InterruptedException {
            // random waiting time is less than 15ms.run randomly to different
            // concurrency results.
            int random = ( int ) ( Math.random() * 15 );
            Thread.sleep( random );
            try {
                s3Client2.deleteBucket( bucketName );
            } catch ( AmazonS3Exception e ) {
                int errCode = e.getStatusCode();
                saveResult( errCode, e );
            } finally {
                if ( s3Client2 != null ) {
                    s3Client2.shutdown();
                }
            }
        }
    }

}
