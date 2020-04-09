package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * test content: 带前缀prefix和delimiter查询对象版本列表，匹配对象无对应目录 testlink-case:seqDB-18144
 *
 * @author wangkexin
 * @Date 2019.05.05
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18144 extends S3TestBase {
    private String bucketName = "bucket18144";
    private String[] keyName = { "dir1/test18144_1",
            "dir1/Dir2/dir3/test18144_2", "dir1/test18144_3",
            "dir1/dir2/aa/test18144_4", "testdir18144.txt" };
    private List< String > versionsKeys = new ArrayList< String >(
            Arrays.asList( "dir1/test18144_1", "dir1/Dir2/dir3/test18144_2",
                    "dir1/test18144_3", "dir1/dir2/aa/test18144_4" ) );
    private String delimiter = "?";
    private String prefix = "dir1";
    private int versionNum = 3;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        // put multiple objects
        for ( String objectName : keyName ) {
            for ( int j = 0; j < versionNum; j++ ) {
                s3Client.putObject( bucketName, objectName,
                        "object_file18144" );
            }
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 查看访问计划索引为目录索引
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix ) );
        List< String > expCommPrefixes = new ArrayList<>();

        MultiValueMap< String, String > expVersionsMap = new LinkedMultiValueMap< String, String >();
        Collections.sort( versionsKeys );
        for ( int i = 0; i < versionsKeys.size(); i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expVersionsMap.add( versionsKeys.get( i ),
                        String.valueOf( j ) );
            }
        }

        if ( !versionList.isTruncated() ) {
            ObjectUtils.checkListVSResults( versionList, expCommPrefixes,
                    expVersionsMap );
        } else {
            Assert.fail( "vsList.isTruncated() must be false" );
        }
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
