package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
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
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-18195: concurrent create and delete the different
 *              objects,the objects uses the same dir
 *
 * @author wuyan
 * @Date 2019.5.8
 * @version 1.00
 */
public class CreateAndDeleteOjbects18195 extends S3TestBase {
    private boolean runSuccess = false;
    private String delimiter = "%";
    private String bucketName = "bucket18195";
    private String keyName = "/目录1/object18195";
    private int objectNums = 20;
    private List<String> deleteKeyList = new ArrayList<>();
    private List<String> createKeyList = new ArrayList<>();
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        for ( int i = 0; i < objectNums; i++ ) {
            String subKeyName = keyName + "_" + i + delimiter + "test.png";
            s3Client.putObject( bucketName, subKeyName, keyName + "_" + i );
            deleteKeyList.add( subKeyName );
        }
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();

        for ( String subKeyName : deleteKeyList ) {
            threadExec.addWorker( new DeleteObject( subKeyName ) );
        }

        for ( int i = 0; i < objectNums; i++ ) {
            String subKeyName =
                    keyName + "_" + i + delimiter + "addObject/test.png";
            threadExec.addWorker( new CreateObject( subKeyName ) );
            createKeyList.add( subKeyName );
        }

        threadExec.run();

        // check delete object result
        checkCreateAndDeleteObjectResult();
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

    private void checkCreateAndDeleteObjectResult() throws IOException {
        Collections.sort( createKeyList );
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List<S3VersionSummary> versionSummary = versionList
                .getVersionSummaries();

        int existObjectNum = objectNums;
        int actVersionNum = versionSummary.size();
        Assert.assertEquals( actVersionNum, existObjectNum );

        String expVersionId = "null";
        String expFileMd5 = TestTools.getMD5( filePath );
        for ( int i = 0; i < actVersionNum; i++ ) {
            String keyName = versionSummary.get( i ).getKey();
            String versionId = versionSummary.get( i ).getVersionId();
            String etag = versionSummary.get( i ).getETag();
            boolean isDeleteMarker = versionSummary.get( i ).isDeleteMarker();

            Assert.assertEquals( keyName, createKeyList.get( i ) );
            Assert.assertEquals( isDeleteMarker, false );
            Assert.assertEquals( versionId, expVersionId );
            Assert.assertEquals( etag, expFileMd5 );
        }
    }

    private class CreateObject {
        private String keyName;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        private CreateObject( String keyName ) {
            this.keyName = keyName;
        }

        @ExecuteOrder(step = 1)
        private void createObject() {
            s3Client1.putObject( bucketName, keyName, new File( filePath ) );
        }

        @ExecuteOrder(step = 2)
        private void checkResult() throws Exception {
            try {
                String downfileMd5 = ObjectUtils
                        .getMd5OfObject( s3Client1, localPath, bucketName,
                                keyName );
                Assert.assertEquals( downfileMd5,
                        TestTools.getMD5( filePath ) );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private class DeleteObject {
        private String keyName;
        private AmazonS3 s3Client2 = CommLib.buildS3Client();

        private DeleteObject( String keyName ) {
            this.keyName = keyName;
        }

        @ExecuteOrder(step = 1)
        private void deleteObject() {
            s3Client2.deleteObject( bucketName, keyName );
        }

        @ExecuteOrder(step = 2)
        private void checkDeleteResult() {
            try {
                boolean isExistObject = s3Client
                        .doesObjectExist( bucketName, keyName );
                Assert.assertFalse( isExistObject,
                        "the object should not exist! key=" + keyName );
            } finally {
                if ( s3Client2 != null ) {
                    s3Client2.shutdown();
                }
            }
        }
    }
}
