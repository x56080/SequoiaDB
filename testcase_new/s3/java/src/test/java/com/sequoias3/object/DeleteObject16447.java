package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
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
import java.util.Iterator;

/**
 * @Description seqDB-16447: enabling bucket versioning ,create object on the
 *              bucket, than set the bucket version is Suspended, delete object
 * @author wuyan
 * @Date 2018.11.21
 * @version 1.00
 */
public class DeleteObject16447 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16447";
    private String key = "//aa/%maa/bb*中文/object16447";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 300;
    private int updateSize = 1024 * 20;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

    @SuppressWarnings("deprecation")
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
        if ( s3Client.doesBucketExist( bucketName ) ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }

        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        s3Client.putObject( bucketName, key, new File( updatePath ) );
    }

    @Test
    public void testDeleteObject() throws Exception {
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        s3Client.deleteObject( bucketName, key );
        checkDeleteObjectResult( bucketName, key );
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

    private void checkDeleteObjectResult( String bucketName, String key )
            throws Exception {
        // current version object not exist
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertFalse( isExistObject, "the object should not exist!" );

        // deleted object has been a history version object,the versionId is "1"
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key, "1" );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );

        // check the oldest version object,the version is "0"
        String downOldfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key, "0" );
        Assert.assertEquals( downOldfileMd5, TestTools.getMD5( filePath ) );

        // delete the object, add a delete tag ,the object num is 3
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        Iterator< S3VersionSummary > versionIter = versionList
                .getVersionSummaries().iterator();
        int count = 0;
        int deleteTagNums = 0;
        while ( versionIter.hasNext() ) {
            S3VersionSummary vs = versionIter.next();
            String getKey = vs.getKey();
            boolean isDeleteMarker = vs.isDeleteMarker();
            if ( isDeleteMarker ) {
                // the object of delete tag versionid is "null"
                Assert.assertEquals( vs.getVersionId(), "null" );
                deleteTagNums++;
            }
            Assert.assertEquals( getKey, key );
            count++;
        }
        int expDeleteTagNums = 1;
        int expObjectNums = 3;
        Assert.assertEquals( count, expObjectNums );
        Assert.assertEquals( deleteTagNums, expDeleteTagNums );
    }
}
