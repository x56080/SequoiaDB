package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 带prefix、keyMarker、delimiter和versionIdMarker查询对象版本列表，不匹配prefix
 * testlink-case: seqDB-18151
 *
 * @author wangkexin
 * @Date 2019.04.29
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18151 extends S3TestBase {
    private String bucketName = "bucket18151";
    private String[] keyNames = { "dir1%test18151_1", "dir2%dir3%/test18151_2",
            "dir2/test18151_3", "test18151_4" };
    private String delimiter = "%";
    private String prefix = "dir3";
    private String keyMarKer = keyNames[ 0 ];
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( String objectName : keyNames ) {
            s3Client.putObject( bucketName, objectName, "object_file18151" );
        }
        // put object "dir1%test18151_1" again
        s3Client.putObject( bucketName, keyNames[ 0 ], "object_file18151" );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withKeyMarker( keyMarKer ).withVersionIdMarker( "0" )
                .withPrefix( prefix );
        VersionListing versionList = s3Client.listVersions( req );
        Assert.assertEquals( versionList.getCommonPrefixes().size(), 0 );
        Assert.assertEquals( versionList.getVersionSummaries().size(), 0 );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
