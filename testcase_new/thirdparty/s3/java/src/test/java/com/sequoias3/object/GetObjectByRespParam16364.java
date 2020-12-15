package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.ResponseHeaderOverrides;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.amazonaws.util.DateUtils;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

/**
 * @author fanyu
 * @Description:seqDB-16364 :: 获取对象设置Response参数，和上传对象设置Response参数不一致
 * @Date:2018年11月26日
 * @version:1.0
 */

public class GetObjectByRespParam16364 extends S3TestBase {
    private String bucketName = "bucket16364";
    private String objectName = "object16364";
    private AmazonS3 s3Client = null;
    private int fileSize = 204800;
    private File localPath = null;
    private List< String > filePathList = new ArrayList< String >();
    private List< PutObjectResult > objectVSList = new ArrayList< PutObjectResult >();
    private int fileNum = 3;

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
            objectVSList.add( putObject( bucketName, objectName,
                    filePathList.get( i ) ) );
        }

        // random version
        Random random = new Random();
        int randomIndex = random.nextInt( fileNum );
        String randomVersionId = objectVSList.get( randomIndex ).getMetadata()
                .getVersionId();

        Date date = new Date();
        String dateStr = DateUtils.formatRFC822Date( date );
        S3Object object = getObjectByVersion( bucketName, objectName,
                randomVersionId, dateStr );

        // check
        checkResult( object, dateStr, randomIndex );
    }

    @AfterClass
    private void tearDown() {
        try {
            CommLib.clearBucket( s3Client, bucketName );
            TestTools.LocalFile.removeFile( localPath );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkResult( S3Object object, String dateStr, int randomIndex )
            throws Exception {
        ObjectMetadata objectMetadata = object.getObjectMetadata();
        Assert.assertEquals( objectMetadata.getCacheControl(), "CacheControl" );
        Assert.assertEquals( objectMetadata.getContentEncoding(), "UTF-8" );
        Assert.assertEquals( objectMetadata.getContentLanguage(), "English" );
        Assert.assertEquals( objectMetadata.getContentDisposition(),
                "Disposition" );
        Assert.assertEquals( objectMetadata.getContentType(), "text/plain" );
        Assert.assertEquals(
                DateUtils.formatRFC822Date(
                        object.getObjectMetadata().getHttpExpiresDate() ),
                dateStr );
        String tmpPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        S3ObjectInputStream s3ObjectInputStream = null;
        try {
            s3ObjectInputStream = object.getObjectContent();
            ObjectUtils.inputStream2File( object.getObjectContent(), tmpPath );
        } finally {
            if ( s3ObjectInputStream != null ) {
                s3ObjectInputStream.close();
            }
        }
        Assert.assertEquals( TestTools.getMD5( tmpPath ),
                TestTools.getMD5( filePathList.get( randomIndex ) ) );
    }

    private S3Object getObjectByVersion( String bucketName, String key,
            String versionId, String date ) {
        GetObjectRequest request = new GetObjectRequest( bucketName, key );
        ResponseHeaderOverrides overrideHeaders = new ResponseHeaderOverrides();
        overrideHeaders.setCacheControl( "CacheControl" );
        overrideHeaders.setContentDisposition( "Disposition" );
        overrideHeaders.setContentEncoding( "UTF-8" );
        overrideHeaders.setContentLanguage( "English" );
        overrideHeaders.setContentType( "text/plain" );
        overrideHeaders.setExpires( date );
        request.withResponseHeaders( overrideHeaders );
        request.withVersionId( versionId );
        return s3Client.getObject( request );
    }

    private PutObjectResult putObject( String bucketName, String key,
            String filePath ) {
        PutObjectRequest request = new PutObjectRequest( bucketName, key,
                new File( filePath ) );
        ObjectMetadata metaData = new ObjectMetadata();
        Map< String, String > xMeta = new HashMap< String, String >();
        xMeta.put( "test1", "1234" );
        xMeta.put( "test2", "123343" );
        metaData.setUserMetadata( xMeta );
        metaData.setCacheControl( "CacheControl1" );
        metaData.setContentDisposition( "Disposition1" );
        metaData.setContentEncoding( "RTC" );
        metaData.setContentLanguage( "Chinese" );
        metaData.setExpirationTime( new Date() );
        metaData.addUserMetadata( "meta-1", "12346788" );
        request.withMetadata( metaData );
        return s3Client.putObject( request );
    }
}
