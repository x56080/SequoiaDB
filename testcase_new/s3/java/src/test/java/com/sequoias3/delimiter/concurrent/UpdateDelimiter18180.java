package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 并发更新相同分隔符 testlink-case: seqDB-18180
 *
 * @author wangkexin
 * @Date 2019.05.08
 * @version 1.00
 */

public class UpdateDelimiter18180 extends S3TestBase {
    private String bucketName = "bucket18180";
    private String[] objectNames = { "dir1/dir2/test18180_1?aa.txt",
            "dir1/dir2/test18180_2?bb.txt", "dir1/dir2?test18180_3.txt",
            "test18180_4.txt" };
    private String delimiter = "?";
    private int threadNum = 20;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    "object_file18180" );
        }
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new ThreadUpdateDelimiter18180() );
        }
        es.run();

        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        List<String> expCommonPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        List<String> matchContentsList = ObjectUtils
                .getKeys( objectNames, "", delimiter );
        DelimiterUtils
                .listObjectsWithDelimiter( s3Client, bucketName, delimiter,
                        expCommonPrefixes, matchContentsList );
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

    class ThreadUpdateDelimiter18180 {
        @ExecuteOrder(step = 1, desc = "更新分隔符")
        public void updateDelimiter() {
            DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        }
    }
}
