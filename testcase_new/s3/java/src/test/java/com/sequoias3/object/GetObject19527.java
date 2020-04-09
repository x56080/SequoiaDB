package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.ResponseHeaderOverrides;
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
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;

/**
 * @Description seqDB-19527 : 获取对象指定response属性
 * @author wuyan
 * @Date 2019.09.25
 * @version 1.00
 */
public class GetObject19527 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/bb%/object19527.jps";
    private String bucketName = "bucket19527";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private Date httpExpiresDate = null;

    @BeforeClass
    private void setUp() throws IOException {
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

        s3Client.putObject( bucketName, keyName,
                "testobjectForGetHttpExpiresDate" );
        // set the httpExpiresData
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        Date lastModifiedDate = objMetadata.getLastModified();
        long lastModifiedTime = lastModifiedDate.getTime();
        // set date 30 min later than lastModified time
        long timestamp = lastModifiedTime + 30 * 60 * 1000l;
        httpExpiresDate = new Date( timestamp );
    }

    @Test
    public void testCopyObject() throws Exception {
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        getObjectAndCheckResponseHeaderInfos( bucketName, keyName );

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
            s3Client.shutdown();
        }
    }

    private void getObjectAndCheckResponseHeaderInfos( String bucketName,
            String keyName ) throws Exception {
        GetObjectRequest request = new GetObjectRequest( bucketName, keyName );
        ResponseHeaderOverrides rHeaders = new ResponseHeaderOverrides();
        rHeaders.setContentDisposition( "this is object!" );
        rHeaders.setContentType( "jps" );
        rHeaders.setCacheControl( "RFC2616" );
        rHeaders.setContentEncoding( "tar" );
        rHeaders.setContentLanguage( "zh" );
        rHeaders.setExpires( "test" );
        // the type of date from CST to GMT
        Calendar calendar = Calendar.getInstance();
        calendar.setTime( httpExpiresDate );
        calendar.set( Calendar.HOUR, calendar.get( Calendar.HOUR ) - 8 );
        Date date = calendar.getTime();
        SimpleDateFormat sdf = new SimpleDateFormat(
                "EEE, dd MMM yyyy HH:mm:ss 'GMT'", Locale.ENGLISH );
        String expiresDate = sdf.format( date );
        rHeaders.setExpires( expiresDate );
        request.withResponseHeaders( rHeaders );
        S3Object object = s3Client.getObject( request );

        // check the return responseHeaderInfos
        ObjectMetadata result = object.getObjectMetadata();
        Assert.assertEquals( result.getETag(), TestTools.getMD5( filePath ) );
        Assert.assertEquals( result.getContentLength(), fileSize );
        Assert.assertEquals( result.getContentDisposition(),
                "this is object!" );
        Assert.assertEquals( result.getCacheControl(), "RFC2616" );
        Assert.assertEquals( result.getContentEncoding(), "tar" );
        Assert.assertEquals( result.getContentLanguage(), "zh" );
        Assert.assertEquals( result.getContentType(), "jps" );
        Assert.assertEquals( result.getHttpExpiresDate(), httpExpiresDate );

        // check the content
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}
