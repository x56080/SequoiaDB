package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * test content: 指定delimiter为旧分隔符查询对象列表 testlink-case: seqDB-18135
 *
 * @author wangkexin
 * @Date 2019.04.24
 * @version 1.00
 */

public class ListObjectsWithDelimiter18135 extends S3TestBase {
    private String bucketName = "bucket18135";
    private String[] objectNames = { "dir1?test/18135_1",
            "dir1?dir2??/dir3/test18135_2", "dir1?test/18135_3",
            "dir1?dir2?aa?//test18135_4", "dir1?dir2?aa?cc?test/18135_5",
            "dir1?dir2/?aa?dd?test18135_6", "testdir1.txt" };
    private String oldDelimiter = "/";
    private String newDelimiter = "?";
    private List< String > expCommonprefixes = new ArrayList< String >();
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    "object_file18135" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为?（默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, newDelimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, newDelimiter );

        // 手工校验查询方式为元数据扫描方式
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withDelimiter( oldDelimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult = result.getCommonPrefixes();

        // check result
        expCommonprefixes = ObjectUtils.getCommPrefixes( objectNames, "",
                oldDelimiter );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommonprefixes );

        List< String > contents = ObjectUtils.getKeys( objectNames, "",
                oldDelimiter );
        List< S3ObjectSummary > objectSummaries = result.getObjectSummaries();
        ObjectUtils.checkListObjectsV2KeyName( objectSummaries, contents );

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
