package com.sequoias3.object.serial;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.amazonaws.util.Md5Utils;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.util.Random;

/**
 * test content: 增加对象内容较大 testlink-case: seqDB-16516
 *
 * @author wangkexin
 * @Date 2018.11.15
 * @version 1.00
 */
public class CreateObject16516 extends S3TestBase {
    String bucketName = "bucket16516";
    String keyName = "object16516";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private boolean runSuccess = false;

    public static void createFile( String filePath, BigInteger size )
            throws IOException {
        FileOutputStream fos = null;
        try {
            TestTools.LocalFile.createFile( filePath );
            File file = new File( filePath );
            fos = new FileOutputStream( file );
            BigInteger written = new BigInteger( "0" );
            byte[] fileBlock = new byte[ 1024 ];
            while ( ( written.compareTo( size ) ) == -1 ) {
                new Random().nextBytes( fileBlock );
                BigInteger toWrite = size.subtract( written );
                BigInteger len = BigInteger.valueOf( fileBlock.length )
                        .compareTo( toWrite ) == -1
                                ? BigInteger.valueOf( fileBlock.length )
                                : toWrite;
                fos.write( fileBlock, 0, Integer.valueOf( len.toString() ) );
                written = written.add( len );
            }
        } catch ( IOException e ) {
            System.out.println( "create file failed, file=" + filePath );
            throw e;
        } finally {
            if ( fos != null ) {
                fos.close();
            }
        }
    }

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testPutObject() throws Exception {
        // 上传对象内容大小为2G
        BigInteger fileSize = new BigInteger( "2147483648" );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        createFile( filePath, fileSize );

        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        checkPutObjectResult( filePath, fileSize );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
    }

    private void checkPutObjectResult( String filePath, BigInteger fileSize )
            throws Exception {
        // down file
        GetObjectRequest request = new GetObjectRequest( bucketName, keyName );
        S3Object object = s3Client.getObject( request );
        S3ObjectInputStream s3is = object.getObjectContent();

        Assert.assertEquals( Md5Utils.md5AsBase64( s3is ),
                Md5Utils.md5AsBase64(
                        new FileInputStream( new File( filePath ) ) ),
                "md5 is different!" );
        long actSize = object.getObjectMetadata().getContentLength();
        Assert.assertEquals( new BigInteger( String.valueOf( actSize ) ),
                fileSize, "size is different!" );
    }
}