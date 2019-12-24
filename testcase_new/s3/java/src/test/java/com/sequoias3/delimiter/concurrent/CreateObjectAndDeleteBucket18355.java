package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
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

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

/**
 * @Description seqDB-18345: concurrent create one object and delete bucket.the
 *              object name include delimiter
 *
 * @author wuyan
 * @Date 2019.5.21
 * @version 1.00
 */

public class CreateObjectAndDeleteBucket18355 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18355";
    private String keyName = "dir1/test?18355";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private List<String> expKeyList = Collections
            .synchronizedList( new LinkedList<String>() );

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @SuppressWarnings("deprecation")
    @Test
    public void testObject() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        CreateObject createObject = new CreateObject( keyName );

        DeleteBucket deleteBucket = new DeleteBucket();
        threadExec.addWorker( deleteBucket );
        threadExec.addWorker( createObject );
        threadExec.run();

        int errorDeleteBucketCode = deleteBucket.getRetCode();
        int errorCreateObjectCode = createObject.getRetCode();
        if ( errorDeleteBucketCode == 0 ) {
            // delete bucket success,create object fail.
            Assert.assertEquals( errorCreateObjectCode, 404 );
            Assert.assertFalse( s3Client.doesBucketExist( bucketName ) );
            Assert.assertFalse(
                    s3Client.doesObjectExist( bucketName, keyName ) );
        } else {
            // create object success,delete bucekt fail,the
            // errorCode:409(BucketNotEmpty)
            Assert.assertEquals( errorDeleteBucketCode, 409 );
            Assert.assertEquals( errorCreateObjectCode, 0 );
            Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );
            Assert.assertTrue(
                    s3Client.doesObjectExist( bucketName, keyName ) );
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

    private class CreateObject extends ResultStore {
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
                expKeyList.add( keyName );
            } catch ( AmazonS3Exception e ) {
                int errCode = e.getStatusCode();
                saveResult( errCode, e );
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
            // random waiting time is less than 100ms.run randomly to different
            // concurrency results.
            int random = ( int ) ( Math.random() * 100 );
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
