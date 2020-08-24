package com.sequoias3.object;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: PutObjectRequest接口参数校验 testlink-case: seqDB-16478
 *
 * @author wangkexin
 * @Date 2019.01.07
 * @version 1.00
 */
public class TestPutObjectRequest16478 extends S3TestBase {
    private String bucketName = "bucket16478";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "legalKeyNameProvider")
    public Object[][] generateKeyName() {
        String ascii = new String();
        for ( int i = 1; i < 32; i++ ) {
            ascii += ( char ) i;
        }
        for ( int i = 127; i < 256; i++ ) {
            ascii += ( char ) i;
        }
        return new Object[][] {
                // test a : 范围内取值
                new Object[] { "/dir1/test.txt" },
                // test b : 长度边界值
                new Object[] { ObjectUtils.getRandomString( 1 ) },
                new Object[] { ObjectUtils.getRandomString( 900 ) },
                // test c : 包含特殊字符
                new Object[] { "!-_.*'()" },
                // test d : 包含 数字字符[0-9a-zA-Z]
                new Object[] {
                        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" },
                // test e : 包含需要特殊处理的字符
                new Object[] { "&@:,$=+?;" + ascii + " " },
                // test f : 包含不建议使用的字符
                new Object[] { "\\^`><{}][#%“~|" },
                // test g : 包含中文字符
                new Object[] { "测试对象名" }, };
    }

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test(dataProvider = "legalKeyNameProvider")
    public void testLegalKeyName( String keyName ) throws Exception {
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        PutObjectResult result = s3Client.putObject( new PutObjectRequest(
                bucketName, keyName, new File( filePath ) ) );
        String actMd5 = result.getETag();
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( actMd5, expMd5,
                "md5 is wrong! the key name is : " + keyName );
        TestTools.LocalFile.removeFile( localPath );
        actSuccessTests.getAndIncrement();
    }

    @Test
    public void testIllegalKeyName() throws Exception {
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        // test a : 对象名为空串，null，901个字节
        try {
            s3Client.putObject( new PutObjectRequest( bucketName, "",
                    new File( filePath ) ) );
            Assert.fail( "when key name is '',it should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "MalformedXML" );
        }

        try {
            s3Client.putObject( new PutObjectRequest( bucketName, null,
                    new File( filePath ) ) );
            Assert.fail( "when key name is null,it should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The key parameter must be specified when uploading an object" );
        }

        try {
            s3Client.putObject( new PutObjectRequest( bucketName,
                    ObjectUtils.getRandomString( 901 ),
                    new File( filePath ) ) );
            Assert.fail( "when key name is 901 characters,it should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorMessage(), "Your key is too long." );
        }

        // test b : 桶名为null
        try {
            s3Client.putObject( new PutObjectRequest( null, "/dir/test16478",
                    new File( filePath ) ) );
            Assert.fail( "when bucket name is null,it should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when uploading an object" );
        }

        // test c : file 为不存在的路径、文件名不存在
        String nonexistentFilePath = localPath + File.separator
                + "nonexistentFilePath.txt";
        try {
            s3Client.putObject( new PutObjectRequest( bucketName,
                    "/dir/test16478", new File( nonexistentFilePath ) ) );
            Assert.fail( "when file path does not exist,it should fail" );
        } catch ( SdkClientException e ) {
            Assert.assertTrue(
                    e.getMessage().contains( "No such file or directory" ),
                    e.getMessage() );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == ( generateKeyName().length + 1 ) ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
