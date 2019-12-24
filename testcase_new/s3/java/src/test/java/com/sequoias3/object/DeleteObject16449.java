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
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * test content: 开启版本控制，带versionId删除历史版本对象
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */
public class DeleteObject16449 extends S3TestBase {
    private String bucketName = "bucket16449";
    private String keyName = "testkey16449";
    private List<S3VersionSummary> expVersionList = new ArrayList<>();
    private int oneObjVersionNum = 3;
    private String file = "object16449";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket
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
        // delete object with latest version id
        String historyVersionId = "0";
        s3Client.deleteVersion( bucketName, keyName, historyVersionId );
        expVersionList.remove( oneObjVersionNum - 1 );

        try {
            s3Client.getObject( new GetObjectRequest( bucketName, keyName,
                    historyVersionId ) );
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
