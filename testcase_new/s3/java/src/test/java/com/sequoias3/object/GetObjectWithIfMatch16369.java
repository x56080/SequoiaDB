package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
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

/**
 * @Description seqDB-16369: enabling bucket versioning,get object with ifMatch
 *              and specify versionId
 * @author wuyan
 * @Date 2018.11.14
 * @version 1.00
 */
public class GetObjectWithIfMatch16369 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "aa/bb/object16369";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private int updateSize = 1024 * 40;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );
        s3Client = CommLib.buildS3Client();
        ObjectUtils.deleteObjectAllVersions( s3Client,
                S3TestBase.enableVerBucketName, key );
    }

    @Test
    public void testGetObject() throws Exception {
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( filePath ) );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( updatePath ) );

        String curVersionId = "1";
        String hisVersionId = "0";
        // match curVersion object etag
        getObjectWithIfMatchAndCheckContent( S3TestBase.enableVerBucketName,
                key, curVersionId, updatePath );
        // match hisVersion object etag
        getObjectWithIfMatchAndCheckContent( S3TestBase.enableVerBucketName,
                key, hisVersionId, filePath );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client,
                        S3TestBase.enableVerBucketName, key );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void getObjectWithIfMatchAndCheckContent( String bucketName,
            String key, String versionId, String filePath ) throws Exception {
        String eTag = TestTools.getMD5( filePath );
        GetObjectRequest request = new GetObjectRequest( bucketName, key,
                versionId );
        request.withMatchingETagConstraint( eTag );
        S3Object object = s3Client.getObject( request );
        checkGetObjectResult( object, filePath, versionId );
    }

    private void checkGetObjectResult( S3Object object, String filePath,
            String versionId ) throws Exception {
        S3ObjectInputStream s3is = object.getObjectContent();
        String downloadPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( s3is, downloadPath );
        String getMd5 = TestTools.getMD5( downloadPath );
        Assert.assertEquals( getMd5, TestTools.getMD5( filePath ) );

        // check the versionId
        Assert.assertEquals( object.getObjectMetadata().getVersionId(),
                versionId );
    }
}
