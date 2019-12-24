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
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 同时设置新分隔符和旧分隔符 testlink-case: seqDB-18182
 *
 * @author wangkexin
 * @Date 2019.05.08
 * @version 1.00
 */

public class UpdateDelimiter18182 extends S3TestBase {
    private String bucketName = "bucket18182";
    private String[] objectNames = { "dir1/dir2/test18182_1%aa.txt",
            "dir1/dir2/%test18182_2?bb.txt", "dir1/dir2?test%18182_3.txt",
            "test18182_4.txt" };
    private String delimiter1 = "/";
    private String delimiter2 = "%";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    "object_file18182" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        ThreadUpdateDelimiter18182 updateDelimiterThread1 = new ThreadUpdateDelimiter18182(
                delimiter1 );
        ThreadUpdateDelimiter18182 updateDelimiterThread2 = new ThreadUpdateDelimiter18182(
                delimiter2 );
        es.addWorker( updateDelimiterThread1 );
        es.addWorker( updateDelimiterThread2 );
        es.run();

        // 409 表示当前分隔符状态不稳定 "DelimiterNotStable"
        if ( updateDelimiterThread1.getRetCode() == 0
                && updateDelimiterThread2.getRetCode() == 0
                || updateDelimiterThread1.getRetCode() == 409
                && updateDelimiterThread2.getRetCode() == 0 ) {
            checkResult( delimiter2 );
        } else {
            Assert.fail( "unexpect result , t1.getRetCode()="
                    + updateDelimiterThread1.getRetCode() + ", t2.getRetCode()="
                    + updateDelimiterThread2.getRetCode() );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( String keyName : objectNames ) {
                    s3Client.deleteObject( bucketName, keyName );
                }
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkResult( String delimiter ) throws Exception {
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        List<String> expCommonPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        List<String> matchContentsList = ObjectUtils
                .getKeys( objectNames, "", delimiter );
        DelimiterUtils
                .listObjectsWithDelimiter( s3Client, bucketName, delimiter,
                        expCommonPrefixes, matchContentsList );
    }

    class ThreadUpdateDelimiter18182 extends ResultStore {
        private String delimiter = "";

        public ThreadUpdateDelimiter18182( String delimiter ) {
            this.delimiter = delimiter;
        }

        @ExecuteOrder(step = 1, desc = "更新分隔符")
        public void updateDelimiter() {
            try {
                DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
            } catch ( AmazonS3Exception e ) {
                saveResult( e.getStatusCode(), e );
            }
        }
    }
}
