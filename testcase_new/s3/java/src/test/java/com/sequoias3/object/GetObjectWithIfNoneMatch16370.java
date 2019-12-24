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
 * @Description seqDB-16370: enabling bucket versioning,get object with
 *              ifNoneMatch and specify versionId
 * @author wuyan
 * @Date 2018.11.14
 * @version 1.00
 */
public class GetObjectWithIfNoneMatch16370 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "aa/bb/object16370";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 50;
    private int updateSize = 1024 * 60;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        updatePath =
                localPath + File.separator + "localFile_" + updateSize + ".txt";

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

        // set etag and versionId is curVersion object
        getObjectWithCurVersionEtag( S3TestBase.enableVerBucketName,
                updatePath );
        // set etag is hisVersion object etag, versionId is curVersion
        getObjectWithHisVersionEtag( S3TestBase.enableVerBucketName, filePath );
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

    private void getObjectWithCurVersionEtag( String bucketName,
            String filePath ) throws Exception {
        String eTag = TestTools.getMD5( filePath );
        GetObjectRequest request = new GetObjectRequest( bucketName, key, "1" );
        request.withNonmatchingETagConstraint( eTag );
        S3Object object = s3Client.getObject( request );
        Assert.assertNull( object, "not match get object retrun null!" );
    }

    private void getObjectWithHisVersionEtag( String bucketName,
            String filePath ) throws Exception {
        String eTag = TestTools.getMD5( filePath );
        GetObjectRequest request = new GetObjectRequest( bucketName, key, "1" );
        request.withNonmatchingETagConstraint( eTag );
        // get currentVersion object
        S3Object object = s3Client.getObject( request );
        checkGetObjectResult( object, updatePath, "1" );
    }

    private void checkGetObjectResult( S3Object object, String filePath,
            String versionId ) throws Exception {
        S3ObjectInputStream s3is = object.getObjectContent();
        String downloadPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( s3is, downloadPath );
        String getMd5 = TestTools.getMD5( downloadPath );
        Assert.assertEquals( getMd5, TestTools.getMD5( filePath ) );

        // check the versionId
        Assert.assertEquals( object.getObjectMetadata().getVersionId(),
                versionId );
    }
}
