package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.List;

/**
 * test content: 带prefix、delimiter和和start-after查询对象元数据列表，不匹配start-after
 * testlink-case: seqDB-18122
 *
 * @author wangkexin
 * @Date 2019.04.16
 * @version 1.00
 */
public class ListObjectsWithDelimiter18122 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18122";
    private String delimiter = "?";
    private String[] objectNames = { "dir1?test18122_1",
            "dir1?Dir2?test18122_2", "?aa?bb?test18122_3",
            "?aa?cc?test18122_4" };
    private File localPath = null;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 20;
    private String filePath = null;

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
        s3Client.createBucket( bucketName );

        // 将分隔符设置为? （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    new File( filePath ) );
        }
    }

    @Test
    private void testListObjects() throws Exception {
        String startAfter = "key18122";
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( startAfter ).withPrefix( "dir1" )
                .withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< S3ObjectSummary > objects = result.getObjectSummaries();
        List< String > commonPrefixes = result.getCommonPrefixes();
        // misMatchObject, the list size is 0
        Assert.assertEquals( objects.size(), 0 );
        Assert.assertEquals( commonPrefixes.size(), 0 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                TestTools.LocalFile.removeFile( localPath );
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
