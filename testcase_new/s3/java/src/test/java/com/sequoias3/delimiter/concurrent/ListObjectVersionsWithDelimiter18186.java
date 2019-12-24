package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 并发不同条件查询对象版本列表 testlink-case: seqDB-18186
 *
 * @author wangkexin
 * @Date 2019.05.09
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18186 extends S3TestBase {
    private String bucketName = "bucket18186";
    private String keyName = "dir1/dir2/test18186";
    private String delimiter = "?";
    private int objectNum = 100;
    private int versionNum = 3;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private List<String> keyNames = new ArrayList<>();
    private File localPath = null;
    private String filePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + "_" + i + delimiter + ".txt";
            for ( int j = 0; j < versionNum; j++ ) {
                s3Client.putObject( bucketName, currentKey,
                        new File( filePath ) );
            }
            keyNames.add( currentKey );
        }

        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new ThreadListObjectVersions18186() );
        es.addWorker( new ThreadListObjectVersionsWithDelimiter18186() );
        es.run();
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
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    class ThreadListObjectVersions18186 {
        private VersionListing vsList = new VersionListing();
        private String[] objectNames = keyNames
                .toArray( new String[ keyNames.size() ] );

        @ExecuteOrder(step = 1, desc = "不设置筛选条件查询对象版本列表")
        public void ListObjectVersions() {
            vsList = s3Client.listVersions(
                    new ListVersionsRequest().withBucketName( bucketName ) );
        }

        @ExecuteOrder(step = 2, desc = "检查匹配结果")
        public void checkResult() {
            List<String> expCommprefixList = new ArrayList<>();
            // expected versions result
            MultiValueMap<String, String> expVersionsMap = new LinkedMultiValueMap<String, String>();
            for ( int i = 0; i < objectNames.length; i++ ) {
                for ( int j = versionNum - 1; j >= 0; j-- ) {
                    expVersionsMap.add( objectNames[ i ], String.valueOf( j ) );
                }
            }

            Assert.assertFalse( vsList.isTruncated(),
                    "vsList.isTruncated() must be false" );
            ObjectUtils.checkListVSResults( vsList, expCommprefixList,
                    expVersionsMap );
        }
    }

    class ThreadListObjectVersionsWithDelimiter18186 {
        private String[] objectNames = keyNames
                .toArray( new String[ keyNames.size() ] );
        private VersionListing vsList = new VersionListing();

        @ExecuteOrder(step = 1, desc = "指定delimiter查询对象版本列表")
        public void ListObjectVersions() {
            vsList = s3Client.listVersions(
                    new ListVersionsRequest().withBucketName( bucketName )
                            .withDelimiter( delimiter ) );
        }

        @ExecuteOrder(step = 3, desc = "检查指定delimiter查询对象版本列表匹配结果")
        public void checkResult() {
            List<String> expCommprefixList = ObjectUtils
                    .getCommPrefixes( objectNames, "", delimiter );
            // expected versions result
            MultiValueMap<String, String> expVersionsMap = new LinkedMultiValueMap<String, String>();
            Assert.assertFalse( vsList.isTruncated(),
                    "vsList.isTruncated() must be true" );
            ObjectUtils.checkListVSResults( vsList, expCommprefixList,
                    expVersionsMap );
        }
    }
}
