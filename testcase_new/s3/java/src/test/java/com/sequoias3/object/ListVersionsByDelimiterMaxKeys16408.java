package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;

/**
 * @Description: seqDB-16408 :: 带delimiter和maxkeys查询对象版本列表，不匹配delimiter
 * @author fanyu
 * @Date:2018年11月23日
 * @version:1.0
 */

public class ListVersionsByDelimiterMaxKeys16408 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16408";
    private String[] objectNames = { "aaa/16408", "bbb/16408", "ccc16408",
            "ddd16408", "eee16408" };
    private AmazonS3 s3Client = null;
    private int versionNum = 5;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            for ( int j = 0; j < versionNum; j++ ) {
                s3Client.putObject( bucketName, objectName,
                        "" + UUID.randomUUID() );
            }
        }
    }

    @Test
    private void test() throws Exception {
        String delimiter = "#";
        Integer maxResults = versionNum * objectNames.length;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxResults( maxResults ) );
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( String objectName : objectNames ) {
            for ( int i = versionNum - 1; i >= 0; i-- ) {
                expMap.add( objectName, String.valueOf( i ) );
            }
        }
        if ( !vsList.isTruncated() ) {
            ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                    expMap );
        } else {
            Assert.fail( "vsList.isTruncated() must be false" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
