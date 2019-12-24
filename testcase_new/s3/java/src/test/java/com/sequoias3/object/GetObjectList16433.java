package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 带prefix、start-after、delimiter匹配查询对象元数据列表（多次查询） testlink-case:
 * seqDB-16433
 *
 * @author wangkexin
 * @Date 2018.11.15
 * @version 1.00
 */
public class GetObjectList16433 extends S3TestBase {
    private String bucketName = "bucket16433";
    private String keyName = "/dir/dir";
    private String prefix = "/dir/";
    private String delimiter = "/";
    private String startAfter = "/dir/dir2/";
    private List<String> expresultList = new ArrayList<String>();
    private int objectTotalNum = 15;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( int i = 0; i < objectTotalNum; i++ ) {
            String currentKeyName = keyName + i + "/16433";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16433" );
            String commprefix = currentKeyName.substring( 0,
                    currentKeyName.lastIndexOf( delimiter ) + 1 );
            expresultList.add( commprefix );
        }
        Collections.sort( expresultList );
    }

    @Test
    public void testGetObjectList() throws Exception {
        List<String> commprefixesResult = new ArrayList<>();
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter ).withStartAfter( startAfter );
        ListObjectsV2Result result;

        do {
            result = s3Client.listObjectsV2( req );
            commprefixesResult.addAll( result.getCommonPrefixes() );
            String nextContinuationToken = result.getNextContinuationToken();
            req.setContinuationToken( nextContinuationToken );
        } while ( result.isTruncated() );

        // expresultList are stored after 'startAfter'
        // subList[int,int)
        expresultList = expresultList
                .subList( expresultList.indexOf( startAfter ) + 1,
                        expresultList.size() );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expresultList );
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
