package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: 带versionId删除对象，该对象为删除标记
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */

public class DeleteObject16453 extends S3TestBase {
    private String bucketName = "bucket16453";
    private String keyName = "testkey16453";
    private int deleteVersionNum = 3;
    private List< S3VersionSummary > expDeleteMarkersList = new ArrayList< S3VersionSummary >();
    private AmazonS3 s3Client = null;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "removeProvider")
    public Object[][] generateRemoveIndex() {
        return new Object[][] {
                // delete the latest version of deletemarker
                new Object[] { 0 },
                // delete the history version of deletemarker
                new Object[] { 1 } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket status is enabled
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test(dataProvider = "removeProvider")
    public void testGetObjectList( int remove_index ) throws Exception {
        for ( int i = 0; i < deleteVersionNum; i++ ) {
            s3Client.deleteObject( bucketName, keyName );
            S3VersionSummary version = new S3VersionSummary();
            version.setKey( keyName );
            version.setIsDeleteMarker( true );
            // Objects in the version list are stored in reverse order by
            // versionId , like 2,1,0
            version.setVersionId(
                    String.valueOf( ( deleteVersionNum - 1 ) - i ) );
            expDeleteMarkersList.add( version );
        }

        // remove_index ：which index should be deleted from expDeleteMarkersList
        s3Client.deleteVersion( bucketName, keyName,
                expDeleteMarkersList.get( remove_index ).getVersionId() );

        // check the object version list
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List< S3VersionSummary > verList = versionList.getVersionSummaries();
        checkDeleteMarkerResult( verList, remove_index );

        CommLib.deleteAllObjectVersions( s3Client, bucketName );
        expDeleteMarkersList.clear();
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        if ( actSuccessTests.get() == generateRemoveIndex().length ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }

    private void checkDeleteMarkerResult( List< S3VersionSummary > verList,
            int removeIndex ) {
        expDeleteMarkersList.remove( removeIndex );
        Assert.assertEquals( verList.size(), expDeleteMarkersList.size() );
        for ( int i = 0; i < verList.size(); i++ ) {
            Assert.assertEquals( verList.get( i ).getKey(),
                    expDeleteMarkersList.get( i ).getKey() );
            Assert.assertEquals( verList.get( i ).isDeleteMarker(),
                    expDeleteMarkersList.get( i ).isDeleteMarker() );
            Assert.assertEquals( verList.get( i ).getVersionId(),
                    expDeleteMarkersList.get( i ).getVersionId() );
        }
    }
}
