package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
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
 * @Description seqDB-18192: enabling bucket versioning,concurrent update
 *              delimiter and delete objects,the object name include old
 *              delimiter and new delimiter.
 * @author wuyan
 * @Date 2019.5.8
 * @version 1.00
 */
public class UpdateDelimiterAndDeleteOjbects18192 extends S3TestBase {
    private boolean runSuccess = false;
    private String delimiter = "%";
    private String bucketName = "bucket18192";
    private String keyName = "/目录1/object18192";
    private int objectNums = 100;
    private List<String> keyList = new ArrayList<>();
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
            s3Client.putObject( bucketName, subKeyName, keyName + "_" + i );
            keyList.add( subKeyName );
        }
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        UpdateDelimiter updateDelimiter = new UpdateDelimiter();

        for ( String subKeyName : keyList ) {
            threadExec.addWorker( new DeleteObject( subKeyName ) );
        }

        threadExec.addWorker( updateDelimiter );
        threadExec.run();

        // check delete object result
        checkDeleteObjectResult();
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

    private void checkDeleteObjectResult() {
        Collections.sort( keyList );
        // list versions check the object nums/ keyName
        // /versionID/isDeleteMarker
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List<S3VersionSummary> versionSummary = versionList
                .getVersionSummaries();
        int existObjectNum = objectNums * 2;
        int actVersionNum = versionSummary.size();
        Assert.assertEquals( actVersionNum, existObjectNum );
        for ( int i = 0; i < actVersionNum; i++ ) {
            String keyName = versionSummary.get( i ).getKey();
            String versionId = versionSummary.get( i ).getVersionId();
            boolean isDeleteMarker = versionSummary.get( i ).isDeleteMarker();

            if ( i < objectNums ) {
                // list history version object,versionId is "0"
                String historyVersionId = "0";
                Assert.assertEquals( keyName, keyList.get( i ) );
                Assert.assertEquals( isDeleteMarker, false );
                Assert.assertEquals( versionId, historyVersionId );
            } else {
                // list current version object, the object is deleteTag ,the
                // versionId is "1"
                String currentVersionId = "1";
                Assert.assertEquals( keyName, keyList.get( i - objectNums ) );
                Assert.assertEquals( isDeleteMarker, true );
                Assert.assertEquals( versionId, currentVersionId );
            }
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

    private class DeleteObject {
        private String keyName;

        private DeleteObject( String keyName ) {
            this.keyName = keyName;
        }

        @ExecuteOrder(step = 1)
        private void deleteObject() {
            s3Client.deleteObject( bucketName, keyName );
        }

        @ExecuteOrder(step = 2)
        private void checkResult() {
            // check the currentVersion object is not exist.
            boolean isExistObject = s3Client
                    .doesObjectExist( bucketName, keyName );
            Assert.assertFalse( isExistObject,
                    "the object should not exist! key=" + keyName );
        }
    }
}
