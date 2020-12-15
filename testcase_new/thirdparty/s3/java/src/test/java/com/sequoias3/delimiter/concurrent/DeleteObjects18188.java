package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.amazonaws.services.s3.model.VersionListing;
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

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description: 开启版本控制，并发删除相同目录下不同对象 testlink-case: seqDB-18188
 *
 * @author wangkexin
 * @Date 2019.05.09
 * @version 1.00
 */

public class DeleteObjects18188 extends S3TestBase {
    private String bucketName = "bucket18188";
    private String keyName = "dir1/dir2/test18188";
    private String delimiter = "&";
    private int objectNum = 2;
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
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 上传多个对象，对象名中包含分隔符且分解目录相同
        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + delimiter + "_" + i + ".txt";
            s3Client.putObject( bucketName, currentKey, new File( filePath ) );
            keyNames.add( currentKey );
        }

        ThreadExecutor es = new ThreadExecutor();
        for ( String key : keyNames ) {
            es.addWorker( new ThreadDeleteObject18188( key ) );
        }
        es.run();

        // 查询对象列表对象已不存在
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< S3ObjectSummary > objects = result.getObjectSummaries();
        Assert.assertEquals( objects.size(), 0 );

        // 带分隔符查看版本列表，验证删除标记对象映射目录存在
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter ) );
        List< String > expCommprefixes = new ArrayList<>();
        expCommprefixes.add( keyName + delimiter );
        Assert.assertEquals( vsList.getCommonPrefixes(), expCommprefixes );
        Assert.assertEquals( vsList.getVersionSummaries().size(), 0 );

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

    class ThreadDeleteObject18188 {
        private String keyName = "";

        public ThreadDeleteObject18188( String deleteKeyName ) {
            this.keyName = deleteKeyName;
        }

        @ExecuteOrder(step = 1, desc = "删除对象")
        public void DeleteObject() {
            s3Client.deleteObject( bucketName, keyName );
        }
    }
}
