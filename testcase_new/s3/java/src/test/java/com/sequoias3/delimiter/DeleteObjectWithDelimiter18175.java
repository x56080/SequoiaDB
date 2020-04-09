package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectRequest;
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

/**
 * test content: 开启版本控制，带versionId删除历史版本对象 testlink-case: seqDB-18175
 *
 * @author wangkexin
 * @Date 2019.04.29
 * @version 1.00
 */
public class DeleteObjectWithDelimiter18175 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18175";
    private String key = "dir1//dir2/?dir3#object18175";
    private String delimiter = "?";
    private String deleteVersion = "0";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 30;
    private int updateSize = 1024 * 20;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );

        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        // 上传对象，存在3个版本 v0,v1,v2
        s3Client.putObject( bucketName, key, "content18175" );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        s3Client.putObject( bucketName, key, new File( updatePath ) );
    }

    @Test
    public void testDeleteObject() throws Exception {
        // 指定历史版本v0删除对象
        s3Client.deleteVersion( bucketName, key, deleteVersion );
        checkDeleteObjectResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client, bucketName,
                        key );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDeleteObjectResult() throws Exception {
        // 检查指定versionid删除的历史版本对象已不存在
        try {
            s3Client.getObject(
                    new GetObjectRequest( bucketName, key, deleteVersion ) );
            Assert.fail( "the object of version '" + deleteVersion
                    + "' is still exist!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchVersion" );
        }

        // 查看历史元数据表中v0版本对象记录已删除，对象目录未删除
        // 不指定版本获取v2版本对象内容正确
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );
    }
}
