package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description: 并发不同条件查询对象列表 testlink-case: seqDB-18185
 *
 * @author wangkexin
 * @Date 2019.05.08
 * @version 1.00
 */

public class ListObjectWithDelimiter18185 extends S3TestBase {
    private String bucketName = "bucket18185";
    private String keyName = "dir1/dir2/test18185";
    private String delimiter = "?";
    private String unMatchDelimiter = "#";
    private String prefix = "dir1";
    private int objectNum = 100;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private List< String > keyNames = new ArrayList<>();
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
        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + "_" + i + delimiter + "/test"
                    + unMatchDelimiter + ".txt";
            s3Client.putObject( bucketName, currentKey, new File( filePath ) );
            keyNames.add( currentKey );
        }
        Collections.sort( keyNames );

        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new ThreadListObjectWithDelimiter18185() );
        es.addWorker( new ThreadListObjectWithPrefixAndDelimiter18185() );
        es.run();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( String key : keyNames ) {
                    s3Client.deleteObject( bucketName, key );
                }
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    class ThreadListObjectWithDelimiter18185 {
        private ListObjectsV2Result result = new ListObjectsV2Result();

        @ExecuteOrder(step = 1, desc = "不设置筛选条件查询对象列表")
        public void ListObject() {
            ListObjectsV2Request request = new ListObjectsV2Request()
                    .withBucketName( bucketName ).withEncodingType( "url" );
            result = s3Client.listObjectsV2( request );
        }

        @ExecuteOrder(step = 2, desc = "检查匹配结果")
        public void checkResult() {
            Assert.assertEquals( result.getCommonPrefixes().size(), 0 );
            List< S3ObjectSummary > contents = result.getObjectSummaries();
            List< String > actKeys = new ArrayList<>();
            for ( S3ObjectSummary content : contents ) {
                actKeys.add( content.getKey() );
            }
            Assert.assertEquals( actKeys, keyNames );
        }
    }

    class ThreadListObjectWithPrefixAndDelimiter18185 {
        private ListObjectsV2Result result = new ListObjectsV2Result();
        private String[] objectNames = keyNames
                .toArray( new String[ keyNames.size() ] );
        private List< String > expCommprefixList = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );

        @ExecuteOrder(step = 1, desc = "指定prefix和delimiter查询对象列表")
        public void ListObject() {
            ListObjectsV2Request request = new ListObjectsV2Request()
                    .withBucketName( bucketName ).withEncodingType( "url" )
                    .withPrefix( prefix ).withDelimiter( delimiter );
            result = s3Client.listObjectsV2( request );
        }

        @ExecuteOrder(step = 3, desc = "检查指定prefix和delimiter查询对象列表的匹配结果")
        public void checkResult() {
            List< String > commonPrefixes = result.getCommonPrefixes();
            ObjectUtils.checkListObjectsV2Commprefixes( commonPrefixes,
                    expCommprefixList );

            List< S3ObjectSummary > objects = result.getObjectSummaries();
            Assert.assertEquals( objects.size(), 0 );
        }
    }
}
