package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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
 * test content: 禁用版本控制，不带versionId删除对象 testlink-case: seqDB-18173
 *
 * @author wangkexin
 * @Date 2019.04.29
 * @version 1.00
 */
public class DeleteObjectWithDelimiter18173 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18173";
    private String key = "&&aa&%test中文&object18173";
    private String delimiter = "%";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 300;
    private File localPath = null;
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
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        s3Client.putObject( bucketName, key, new File( filePath ) );
    }

    @Test
    public void testDeleteObject() throws Exception {
        s3Client.deleteObject( bucketName, key );
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
        // 检查删除结果，获取对象已不存在，查看对象对应目录还存在
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertFalse( isExistObject, "the object should not exist!" );
        try {
            s3Client.getObject( bucketName, key );
            Assert.fail( "get not exist key must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
    }
}
