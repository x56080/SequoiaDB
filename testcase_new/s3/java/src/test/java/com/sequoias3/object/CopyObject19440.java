package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/**
 * @Description seqDB-19440:setMetadataDirective接口参数校验
 * @author wuyan
 * @Date 2019.09.20
 * @version 1.00
 */
public class CopyObject19440 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "aa%maa/bb%object19440";
    private String destKeyName = "/object19440";
    private String bucketName = "bucket19440";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024;
    private File localPath = null;
    private String filePath = null;
    private Map<String, String> expMeta = new HashMap<>();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );

        expMeta.put( "tag1", "test1" );
        expMeta.put( "tag2", "copyObject" );
        expMeta.put( "tag3", "copy123" );
        PutObjectRequest request = new PutObjectRequest( bucketName, srcKeyName,
                new File( filePath ) );
        ObjectMetadata metaData = new ObjectMetadata();
        metaData.setUserMetadata( expMeta );
        request.withMetadata( metaData );
        s3Client.putObject( request );
    }

    @Test
    public void testCopyObject() throws Exception {
        // set the COPY /REPLACE/NULL have been tested)
        // test invalid:setMetadataDirective is copy
        copyObjectWithInvalidMeta( destKeyName, "Copy" );
        // test invalid:setMetadataDirective is replace
        copyObjectWithInvalidMeta( destKeyName, "replace" );
        // test invalid:setMetadataDirective is test
        copyObjectWithInvalidMeta( destKeyName, "test" );
        // test invalid:setMetadataDirective is ""
        copyObjectWithInvalidMeta( destKeyName, "" );

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

    private void copyObjectWithInvalidMeta( String destKeyName,
            String metadataDirective ) throws Exception {
        try {
            CopyObjectRequest request = new CopyObjectRequest( bucketName,
                    srcKeyName, bucketName, destKeyName );
            request.setMetadataDirective( metadataDirective );
            s3Client.copyObject( request );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidArgument",
                    e.getStatusCode() + e.getErrorMessage() );
        }

        Assert.assertFalse(
                s3Client.doesObjectExist( bucketName, destKeyName ) );
    }
}
