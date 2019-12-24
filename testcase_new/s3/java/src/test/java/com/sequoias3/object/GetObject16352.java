package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ObjectMetadata;
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

/**
 * @Description seqDB-16352: enabling bucket versioning,create multiple objects with the same name,
 *              get the object without specifying the versionId
 * @author wuyan
 * @Date 2018.11.13
 * @version 1.00
 */
public class GetObject16352 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "aa/bb/object16352";
    private String bucketName = "buckete16352";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 5;
    private int updateSize = 1024 * 2;
    private int objectNums = 20;
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
        CommLib.clearBucket( s3Client, bucketName );

        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testGetObject() throws Exception {
        updateObject( bucketName );
        getObjectAndCheckResult( bucketName );
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

    private void updateObject( String bucketName ) throws Exception {
        for ( int i = 0; i < objectNums; i++ ) {
            s3Client.putObject( bucketName, key, new File( filePath ) );
        }
        // create the new Object
        s3Client.putObject( bucketName, key, new File( updatePath ) );
    }

    private void getObjectAndCheckResult( String bucketName ) throws Exception {
        S3Object object = s3Client.getObject( bucketName, key );
        ObjectMetadata metadata = object.getObjectMetadata();
        // check the versionId is maximum versionId:20
        String versionId = metadata.getVersionId();
        String curVersionId = objectNums + "";
        Assert.assertEquals( versionId, curVersionId );

        // check the etag equal to the md5 of the last update content
        String etag = metadata.getETag();
        Assert.assertEquals( etag, TestTools.getMD5( updatePath ) );

        // chect the content
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );
    }
}
