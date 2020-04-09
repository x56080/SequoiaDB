package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-19353:并发复制对象和更新桶分隔符
 * @author wuyan
 * @Date 2019.09.20
 * @version 1.00
 */
public class CopyObject19353 extends S3TestBase {

    private boolean runSuccess = false;
    private String srcKeyName = "/SRC/bb/object19353";
    private String destKeyName = "/dest/bb/object19353";
    private String bucketName = "bucket19353";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private File localPath = null;
    private String filePath = null;
    private String delimiter = "%";
    private int copyObjectNums = 20;
    private List< String > matchKeyList = new ArrayList<>();

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
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        for ( int i = 0; i < copyObjectNums; i++ ) {
            String subDestKeyName = destKeyName + "_" + i + delimiter
                    + "test.png";
            threadExec.addWorker( new CopyObject( subDestKeyName ) );
            matchKeyList.add( destKeyName + "_" + i + delimiter );
        }

        UpdateDelimiter updateDelimiter = new UpdateDelimiter();
        threadExec.addWorker( updateDelimiter );
        threadExec.run();

        // check the dir of object availability
        List< String > expContentList = new ArrayList<>();
        // srcKeyName no match dellimiter
        expContentList.add( srcKeyName );
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                delimiter, matchKeyList, expContentList );

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

    private class CopyObject extends ResultStore {
        private AmazonS3 s3Client1 = CommLib.buildS3Client();
        private String destKeyName;

        private CopyObject( String destKeyName ) {
            this.destKeyName = destKeyName;

        }

        @ExecuteOrder(step = 1)
        private void copyObject() throws Exception {

            CopyObjectRequest request = new CopyObjectRequest( bucketName,
                    srcKeyName, bucketName, destKeyName );
            s3Client1.copyObject( request );

        }

        @ExecuteOrder(step = 2)
        private void checkObjectContent() throws Exception {
            try {
                String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client1,
                        localPath, bucketName, destKeyName );
                Assert.assertEquals( downfileMd5,
                        TestTools.getMD5( filePath ) );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private class UpdateDelimiter {
        @ExecuteOrder(step = 1)
        private void updateDelimiter() {
            DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        }

        @ExecuteOrder(step = 2)
        private void checkUpdateReslut() throws Exception {
            DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
        }
    }
}
