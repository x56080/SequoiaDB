package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
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
import java.util.Collections;
import java.util.List;

/**
 * test content: 带prefix、delimiter和start-after匹配查询对象元数据列表 testlink-case:
 * seqDB-18123
 *
 * @author wangkexin
 * @Date 2019.04.16
 * @version 1.00
 */
public class ListObjectsWithDelimiter18123 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18123";
    private String delimiter = "#";
    private String[] objectNames = { "dir1#test18123_1",
            "dir1#dir2#/#dir3#test18123_2", "dir1#test18123_3",
            "dir1#dir2#aa#test18123_4", "dir1#dir2#aa#cc#test18123_5",
            "dir1#dir2#aa#dd#test18123_6", "dir1#testdir18123.txt" };
    private String perfix = "dir1";
    private File localPath = null;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 20;
    private String filePath = null;

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
        s3Client.createBucket( bucketName );

        // 将分隔符设置为# （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    new File( filePath ) );
        }
    }

    @Test
    private void testListObjects() throws Exception {
        String startAfter = "dir0#";
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( startAfter ).withPrefix( perfix )
                .withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        List<String> commonPrefixes = result.getCommonPrefixes();

        List<String> expCommonPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, perfix, delimiter );
        Collections.sort( expCommonPrefixes );
        Collections.sort( commonPrefixes );
        Assert.assertEquals( commonPrefixes, expCommonPrefixes );
        Assert.assertEquals( objects.size(), 0 );

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
