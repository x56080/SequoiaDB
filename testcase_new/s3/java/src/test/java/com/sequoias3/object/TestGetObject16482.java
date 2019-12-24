package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;

/**
 * test content: GetObject接口range参数校验 testlink-case: seqDB-16482
 *
 * @author wangkexin
 * @Date 2019.01.08
 * @version 1.00
 */
public class TestGetObject16482 extends S3TestBase {
    private String bucketName = "bucket16482";
    private String keyName = "key16482";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
    }

    @Test
    public void testGetObject() throws Exception {
        // test a : 合法值校验
        String downloadPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        String tmpPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        int start = 0;
        int end = 1024 * 100;

        S3Object object = s3Client.getObject(
                new GetObjectRequest( bucketName, keyName )
                        .withRange( start, end ) );
        ObjectUtils.inputStream2File( object.getObjectContent(), downloadPath );
        seekFile( new FileInputStream( new File( filePath ) ), tmpPath, start,
                end );
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( tmpPath ) );

        // test b : 非法值校验 取值为null
        try {
            s3Client.getObject( null );
            Assert.fail( "when GetObjectRequest is null , it should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "GetObjectRequest cannot be null" );
        }

        // test c : 非法值校验 start超过文件范围
        try {
            s3Client.getObject( new GetObjectRequest( bucketName, keyName )
                    .withRange( fileSize + 1, fileSize + 2 ) );
            Assert.fail( "when 'start' exceeds file scope , it should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorMessage(),
                    "Requested range not satisfiable." );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
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

    private String seekFile( InputStream inputStream, String downloadPath,
            int start, int end ) throws Exception {
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream( new File( downloadPath ), true );
            byte[] read_buf = new byte[ end - start + 1 ];
            int read_len = 0;
            if ( start != 0 ) {
                inputStream.skip( start );
            }
            int count = 0;
            while ( ( read_len = inputStream.read( read_buf ) ) > -1
                    && count < end - start + 1 ) {
                fos.write( read_buf, 0, read_len );
                count += read_len;
            }
        } finally {
            if ( inputStream != null ) {
                inputStream.close();
            }
            if ( fos != null ) {
                fos.close();
            }
        }
        return downloadPath;
    }
}
