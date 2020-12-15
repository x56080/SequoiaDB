package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
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
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * @Description: seqDB-16362 :: 带versionId获取对象，指定range范围
 * @author fanyu
 * @Date:2018年11月12日
 * @version:1.0
 */

public class GetObjectByRange16362 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16362";
    private String objectName = "object16362";
    private AmazonS3 s3Client = null;
    private int fileSize = 204800;
    private File localPath = null;
    private List< String > filePathList = new ArrayList< String >();
    private List< PutObjectResult > objectVSList = new ArrayList< PutObjectResult >();
    private int fileNum = 9;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        String filePath = null;
        for ( int i = 0; i < fileNum; i++ ) {
            filePath = localPath + File.separator + "localFile_"
                    + ( fileSize + i ) + ".txt";
            TestTools.LocalFile.createFile( filePath, fileSize + i );
            filePathList.add( filePath );
        }
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void test() throws Exception {
        for ( int i = 0; i < fileNum; i++ ) {
            objectVSList
                    .add( s3Client.putObject( new PutObjectRequest( bucketName,
                            objectName, new File( filePathList.get( i ) ) ) ) );
        }

        // random version
        Random random = new Random();
        int randomIndex = random.nextInt( fileNum );
        int currfileSize = fileSize + randomIndex;
        String downloadPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        String tmpPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        String randomVersionId = objectVSList.get( randomIndex ).getVersionId();
        int interval = 1024 * 100;
        int start = 0;
        int end = interval;
        for ( int i = 0; i < currfileSize / interval; i++ ) {
            S3Object object = s3Client
                    .getObject( new GetObjectRequest( bucketName, objectName )
                            .withVersionId( randomVersionId )
                            .withRange( start, end ) );
            ObjectUtils.inputStream2File( object.getObjectContent(),
                    downloadPath );
            seekFile(
                    new FileInputStream(
                            ( new File( filePathList.get( randomIndex ) ) ) ),
                    tmpPath, start, end );
            Assert.assertEquals( TestTools.getMD5( downloadPath ),
                    TestTools.getMD5( tmpPath ) );
            if ( i < currfileSize / interval - 1 ) {
                start = end + 1;
                if ( i == ( currfileSize / interval - 2 ) ) {
                    end = currfileSize;
                } else {
                    end = end + interval;
                }
            }
        }
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( filePathList.get( randomIndex ) ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
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
