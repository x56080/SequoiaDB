package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18191: concurrent update delimiter and create objects,the
 *              object name include new delimiter
 * @author wuyan
 * @Date 2019.5.8
 * @version 1.00
 */
public class UpdateDelimiterAndCreateOjbects18191 extends S3TestBase {
    private boolean runSuccess = false;
    private String delimiter = "&";
    private String bucketName = "bucket18191";
    private String keyName = "object18191";
    private int objectNums = 20;
    private List<String> matchKeyList = new ArrayList<>();
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        UpdateDelimiter updateDelimiter = new UpdateDelimiter();

        for ( int i = 0; i < objectNums; i++ ) {
            String subKeyName = keyName + "_" + i + delimiter + "test.png";
            threadExec.addWorker( new CreateObject( subKeyName ) );
            matchKeyList.add( keyName + "_" + i + delimiter );
        }

        threadExec.addWorker( updateDelimiter );
        threadExec.run();

        // check the dir of object availability
        List<String> expContentList = new ArrayList<>();
        DelimiterUtils
                .listObjectsWithDelimiter( s3Client, bucketName, delimiter,
                        matchKeyList, expContentList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private class UpdateDelimiter {
        @ExecuteOrder(step = 1)
        private void updateDelimiter() {
            DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        }

        @ExecuteOrder(step = 2)
        private void checkUpdateResult() throws Exception {
            DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
        }
    }

    private class CreateObject {
        private String keyName;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();
        private String content = "testcontext18191";
        private String expMd5 = TestTools.getMD5( content.getBytes() );
        private PutObjectResult object;

        private CreateObject( String keyName ) {
            this.keyName = keyName;
        }

        @ExecuteOrder(step = 1)
        private void createObject() {
            try {
                object = s3Client1.putObject( bucketName, keyName, content );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }

        @ExecuteOrder(step = 2)
        private void checkResult() {
            // check the content of the create object
            Assert.assertEquals( object.getETag(), expMd5 );
        }
    }
}
