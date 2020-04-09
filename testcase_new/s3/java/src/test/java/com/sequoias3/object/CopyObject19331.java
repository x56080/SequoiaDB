package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
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
import java.util.Date;

/**
 * @Description seqDB-19331:复制对象指定ifModifiedSince条件
 * @author wuyan
 * @Date 2019.09.19
 * @version 1.00
 */
public class CopyObject19331 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19331";
    private String srcKeyName = "/src/bb%/object19331";
    private String destKeyName = "/dest/object19331";
    private AmazonS3 s3Client = null;
    private int fileSize1 = 1024 * 2;
    private int fileSize2 = 1;
    private File localPath = null;
    private String hisVersionFilePath = null;
    private String curVersionFilePath = null;
    private long lastModifiedTime = 0;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        hisVersionFilePath = localPath + File.separator + "localFile_"
                + fileSize1 + ".txt";
        curVersionFilePath = localPath + File.separator + "localFile_"
                + fileSize1 + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( hisVersionFilePath, fileSize1 );
        TestTools.LocalFile.createFile( curVersionFilePath, fileSize2 );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, srcKeyName,
                new File( hisVersionFilePath ) );
        s3Client.putObject( bucketName, srcKeyName,
                new File( curVersionFilePath ) );
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                bucketName, srcKeyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        Date lastModifiedDate = objMetadata.getLastModified();
        lastModifiedTime = lastModifiedDate.getTime();
    }

    @Test
    public void testCopyObject() throws Exception {
        // test c:the currentVersion of sourceObject has not been modified after
        // the date
        copyObjectWithModifiedSinceC();

        // set date an hour early at the lastModified time
        long timestamp = lastModifiedTime - 60 * 60 * 1000l;
        Date date = new Date( timestamp );
        // test a: the hisVersion of sourceObject has been modified after the
        // date
        copyObjectWithModifiedSinceA( date );
        // test b: the currentVersion of sourceObject has been modified after
        // the date
        copyObjectWithModifiedSinceB( date );

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

    private void copyObjectWithModifiedSinceA( Date date ) throws Exception {
        String hisVersionId = "0";
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, hisVersionId, bucketName, destKeyName );
        request.withModifiedSinceConstraint( date );
        s3Client.copyObject( request );

        // check the content of destObject
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, destKeyName, hisVersionId );
        Assert.assertEquals( downfileMd5,
                TestTools.getMD5( hisVersionFilePath ) );
    }

    private void copyObjectWithModifiedSinceB( Date date ) throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, destKeyName );
        request.withModifiedSinceConstraint( date );
        s3Client.copyObject( request );

        // check the content of destObject
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, destKeyName );
        Assert.assertEquals( downfileMd5,
                TestTools.getMD5( curVersionFilePath ) );
    }

    private void copyObjectWithModifiedSinceC() throws Exception {
        // set date 10 minutes later than lastModified time
        long timestamp = lastModifiedTime + 10 * 60 * 1000l;
        Date date = new Date( timestamp );

        try {
            CopyObjectRequest request = new CopyObjectRequest( bucketName,
                    srcKeyName, bucketName, destKeyName );
            request.withModifiedSinceConstraint( date );
            s3Client.copyObject( request );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 304,
                    e.getErrorCode() + e.getErrorMessage() + "\ndate:" + date );
        }
        Assert.assertFalse( s3Client.doesObjectExist( bucketName, destKeyName ),
                "the destObject does not exist!" );
    }
}
