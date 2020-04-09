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
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 开启版本控制，带versionId删除对象不存在
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */
public class DeleteObject16452 extends S3TestBase {
    private String bucketName = "bucket16452";
    private String keyName = "testkey16452";
    private int oneObjVersionNum = 3;
    private String file = "object16452";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket status is enabled
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        for ( int i = 0; i < oneObjVersionNum; i++ ) {
            s3Client.putObject( bucketName, keyName, file + "." + i );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // delete object with unmatched key value
        s3Client.deleteVersion( bucketName, "nonexitkeyname16452", "0" );

        // delete object with unmatched version-id value
        s3Client.deleteVersion( bucketName, keyName, "-1" );

        // check the object version list
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List< S3VersionSummary > verList = versionList.getVersionSummaries();
        Assert.assertEquals( verList.size(), oneObjVersionNum );
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
