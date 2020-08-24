package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description: 指定encoding-type查询对象元数据列表 testlink-case: seqDB-16440
 *
 * @author wangkexin
 * @Date 2018.11.22
 * @version 1.00
 */
public class GetObjectList16440 extends S3TestBase {
    private String bucketName = "bucket16440";
    // the last one of keyNames is belongs to contents
    private String[] keyNames = { "%6Ftest!_ST.-(test0|0.txt",
            "%6Ftest!_、/abc*st/ab）|1.txt", "%6Ftest!_@#$%~^@><|2.txt" };
    private String contentKey = "%6Ftest!_content";
    private String prefix = "%6Ftest!_";
    private String delimiter = "|";
    private List< String > expCommprefixList = new ArrayList< String >( 3000 );
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < keyNames.length; i++ ) {
            String expCommPrefixes = keyNames[ i ].substring( 0,
                    keyNames[ i ].indexOf( delimiter ) + 1 );
            s3Client.putObject( bucketName, keyNames[ i ], "object_file16440" );
            expCommprefixList.add( expCommPrefixes );
        }
        s3Client.putObject( bucketName, contentKey, "object_file16440" );
    }

    @Test
    public void testGetObject() throws Exception {
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withPrefix( prefix ).withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        Assert.assertEquals( result.getEncodingType(), "url" );
        List< String > commprefixesResult = result.getCommonPrefixes();
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommprefixList );

        List< S3ObjectSummary > content = result.getObjectSummaries();
        Assert.assertEquals( content.get( 0 ).getKey(), contentKey,
                "contents is wrong" );
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
