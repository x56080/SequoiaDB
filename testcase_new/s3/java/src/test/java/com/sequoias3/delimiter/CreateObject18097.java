package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
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
import java.util.List;

/**
 * test content: 设置分隔符，新增对象 testlink-case: seqDB-18097
 *
 * @author wangkexin
 * @Date 2019.04.13
 * @version 1.00
 */
public class CreateObject18097 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18097";
    private String keyName = "aa?bb?test18097_1.png";
    private String keyName2 = "aa?bb?test18097_2.png";
    private String expCommPerfix = "aa?";
    private String delimiter = "?";
    private int fileSize = 1024 * 2;
    private File localPath = null;
    private String filePath = null;
    private AmazonS3 s3Client = null;

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
    }

    @Test
    private void testCreateObject() throws Exception {
        // 将分隔符设置为? （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        checkResult();

        // 再次创建相同目录的对象
        s3Client.putObject( bucketName, keyName2, new File( filePath ) );
        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
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

    private void checkResult() throws Exception {
        S3Object obj = s3Client.getObject( bucketName, keyName );
        S3ObjectInputStream s3is = obj.getObjectContent();
        String downloadPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( s3is, downloadPath );
        s3is.close();
        String actMd5 = TestTools.getMD5( downloadPath );
        String expMd5 = TestTools.getMD5( filePath );

        Assert.assertEquals( obj.getKey(), keyName );
        Assert.assertEquals( actMd5, expMd5 );

        // 通过携带delimiter查询对象列表的对外映射场景检测目录表是否生成新目录，对象元数据表和目录表中数据通过连接db手工校验
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), expCommPerfix );
        Assert.assertEquals( result.getObjectSummaries().size(), 0 );
    }
}
