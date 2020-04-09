package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
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
import java.util.Date;

/**
 * @Description seqDB-16340: create objectA on the bucket,enabling bucket
 *              versioning, update the objectA
 * @author wuyan
 * @Date 2018.11.13
 * @version 1.00
 */
public class UpdateObjectWithVersion16340 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "aa/bb/object16340";
    private String bucketName = "bucket16340";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private int updateSize = 1024 * 500;
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
        CommLib.clearBucket( s3Client, bucketName );

        s3Client.createBucket( bucketName );
    }

    @Test
    public void testCreateObject() throws Exception {
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, new File( updatePath ) );
        checkUpdateObjectReslut( bucketName );
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
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkUpdateObjectReslut( String bucketName ) throws Exception {
        // get the new object content is the update content
        S3Object object = s3Client.getObject( bucketName, keyName );
        Date updateDate = object.getObjectMetadata().getLastModified();
        String newVersionId = object.getObjectMetadata().getVersionId();

        // get the create object is the history version,the versionId is null
        GetObjectRequest request = new GetObjectRequest( bucketName, keyName,
                "null" );
        S3Object oldObject = s3Client.getObject( request );
        Date createDate = oldObject.getObjectMetadata().getLastModified();

        // check the newobject versionId, should be 1
        String updateVersionId = "1";
        Assert.assertEquals( newVersionId, updateVersionId );

        // check the modify date
        if ( updateDate.getTime() < createDate.getTime() ) {
            Assert.fail(
                    "updateDate must be grater than createDate! updateDate:"
                            + updateDate.getTime() + "\t createDate:"
                            + createDate.getTime() );
        }

        checkObjectContent( bucketName );
    }

    private void checkObjectContent( String bucketName ) throws Exception {
        String createVersionId = "null";
        String updateVersionId = "1";

        // check the content of the create object
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName, createVersionId );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        // check the content of the update object
        String updateMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName, updateVersionId );
        Assert.assertEquals( updateMd5, TestTools.getMD5( updatePath ) );
    }
}
