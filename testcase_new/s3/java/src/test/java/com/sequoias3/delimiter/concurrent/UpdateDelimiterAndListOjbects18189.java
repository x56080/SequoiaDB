package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
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
 * @Description seqDB-18189: concurrent update delimiter and list objects
 * @author wuyan
 * @Date 2019.5.8
 * @version 1.00
 */
public class UpdateDelimiterAndListOjbects18189 extends S3TestBase {
    private boolean runSuccess = false;
    private String delimiter = "%";
    private String bucketName = "bucket18189";
    private String keyName = "/aa/object18189";
    private int objectNums = 200;
    private List<String> matchKeyList = new ArrayList<>();
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < objectNums; i++ ) {
            String subKeyName = keyName + "_" + i + delimiter + "test.png";
            s3Client.putObject( bucketName, subKeyName,
                    "testcontext18189_" + i );
            matchKeyList.add( keyName + "_" + i + delimiter );
        }
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        UpdateDelimiter updateDelimiter = new UpdateDelimiter();
        ListObjects listObjects = new ListObjects();
        threadExec.addWorker( updateDelimiter );
        threadExec.addWorker( listObjects );
        threadExec.run();

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

    private class ListObjects {
        private List<String> commonPrefixes;
        private List<S3ObjectSummary> objects;

        @ExecuteOrder(step = 1)
        private void listVersions() {
            ListObjectsV2Request request = new ListObjectsV2Request()
                    .withBucketName( bucketName ).withEncodingType( "url" );
            request.withDelimiter( delimiter );
            ListObjectsV2Result result = s3Client.listObjectsV2( request );
            commonPrefixes = result.getCommonPrefixes();
            objects = result.getObjectSummaries();
        }

        @ExecuteOrder(step = 2)
        private void checkListVersions() {
            List<String> expContentList = new ArrayList<>();
            Collections.sort( matchKeyList );
            Assert.assertEquals( commonPrefixes, matchKeyList,
                    "actPrefixes:" + commonPrefixes.toString()
                            + "\n ecpPrefixes:" + matchKeyList.toString() );
            // objects do not match delimiter are displayed in contents,num is
            // 10
            List<String> actContentsList = new ArrayList<>();
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                actContentsList.add( key );
            }

            // check the keyName
            Collections.sort( expContentList );
            Assert.assertEquals( actContentsList, expContentList,
                    "actcontent:" + actContentsList.toString() );
        }
    }
}
