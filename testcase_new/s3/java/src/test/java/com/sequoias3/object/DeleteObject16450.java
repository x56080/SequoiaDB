package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
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
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 禁用版本控制，带versionId删除最新版本对象
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */
public class DeleteObject16450 extends S3TestBase {
    private String bucketName = "bucket16450";
    private String keyName = "testkey16450";
    private List<S3VersionSummary> expVersionList = new ArrayList<>();
    private int oneObjVersionNum = 3;
    private String file = "object16450";
    private File localPath = null;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );

        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket status is enabled
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        for ( int i = 0; i < oneObjVersionNum; i++ ) {
            s3Client.putObject( bucketName, keyName, file + "." + i );
            S3VersionSummary version = new S3VersionSummary();
            version.setKey( keyName );
            // Objects in the version list are stored in reverse order by
            // versionId , like 2,1,0
            version.setVersionId(
                    String.valueOf( ( oneObjVersionNum - 1 ) - i ) );
            expVersionList.add( version );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // set bucket status is suspended
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        // delete object with latest version id
        String latestVersionId = String.valueOf( oneObjVersionNum - 1 );
        s3Client.deleteVersion( bucketName, keyName, latestVersionId );
        expVersionList.remove( 0 );

        try {
            s3Client.getObject( new GetObjectRequest( bucketName, keyName,
                    latestVersionId ) );
            Assert.fail( "the object still exist!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchVersion" );
        }

        // check the object version list
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List<S3VersionSummary> verList = versionList.getVersionSummaries();
        Assert.assertEquals( verList.size(), expVersionList.size() );
        for ( int i = 0; i < verList.size(); i++ ) {
            Assert.assertEquals( verList.get( i ).getKey(),
                    expVersionList.get( i ).getKey() );
            Assert.assertEquals( verList.get( i ).getVersionId(),
                    expVersionList.get( i ).getVersionId() );
        }

        // check that the current latest version of the object is correct by the
        // MD5 value
        String currlatestVersionId = String.valueOf( oneObjVersionNum - 2 );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools
                .getMD5( ( file + "." + currlatestVersionId ).getBytes() ) );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }
}
