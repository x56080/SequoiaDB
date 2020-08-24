package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
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
 * @Description: 禁用版本控制，设置分隔符新增对象 testlink-case: seqDB-18099
 *
 * @author wangkexin
 * @Date 2019.04.13
 * @version 1.00
 */
public class CreateObject18099 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18099";
    private String keyName = "aa*bb**test18099.png";
    private String expCommPerfix = "aa*";
    private String delimiter = "*";
    private int fileSize = 1024 * 2;
    private File localPath = null;
    private String filePath = null;
    private AmazonS3 s3Client = null;

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
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
    }

    @Test
    private void testCreateObject() throws Exception {
        // 将分隔符设置为* （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        s3Client.putObject( bucketName, keyName, new File( filePath ) );
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
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        // 通过携带delimiter查询对象列表的对外映射场景检测目录表是否生成新目录，对象元数据表和目录表中数据通过连接db手工校验
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< String > commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), expCommPerfix );
        Assert.assertEquals( result.getObjectSummaries().size(), 0 );
    }
}
