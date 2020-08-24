package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description: 并发增加相同对象 testlink-case: seqDB-18183
 *
 * @author wangkexin
 * @Date 2019.05.08
 * @version 1.00
 */

public class CreateObject18183 extends S3TestBase {
    private String bucketName = "bucket18183";
    private String keyName = "dir1/dir2/test18183_1?aa?bb?cc?.txt";
    private String delimiter = "?";
    private int threadNum = 20;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private File localPath = null;
    private String filePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new ThreadPutObject18183() );
        }
        es.run();
        List< String > expCommprefixList = new ArrayList<>();
        expCommprefixList.add( "dir1/dir2/test18183_1?" );
        List< String > expContentList = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                delimiter, expCommprefixList, expContentList );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( bucketName, keyName );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    class ThreadPutObject18183 {
        private AmazonS3 s3Client = CommLib.buildS3Client();

        @ExecuteOrder(step = 1, desc = "上传对象")
        public void putObject() {
            try {
                s3Client.putObject( bucketName, keyName, new File( filePath ) );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
