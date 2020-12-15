package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * @Description: 不开启版本控制，不带versionId删除对象 testlink-case: seqDB-18170
 *
 * @author wangkexin
 * @Date 2019.04.29
 * @version 1.00
 */
public class DeleteObjectWithDelimiter18170 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18170";
    private String key = "dir1/dir2/中文&object18170";
    private String delimiter = "&";
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
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testDeleteObject() throws Exception {
        s3Client.deleteObject( bucketName, key );
        // 删除对象后手工查看目录表中对象对应目录也被删除
        checkDeleteObjectResult( bucketName, key );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( bucketName, key );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDeleteObjectResult( String bucketName, String key ) {
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
