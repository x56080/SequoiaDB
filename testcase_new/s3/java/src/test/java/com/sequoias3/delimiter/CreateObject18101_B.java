package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;

/**
 * test content: 开启版本控制，增加同名对象 testlink-case: seqDB-18101
 *
 * @author wangkexin
 * @Date 2019.04.15
 * @version 1.00
 */
public class CreateObject18101_B extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18101b";
    private String keyName = "/aa/!aab/!atest.png";
    private String expCommPerfix = "/aa/!a";
    private String delimiter = "/!a";
    private List< String > expEtags = new ArrayList<>();
    private int putSameObjTimes = 200;
    private File localPath = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    private void testCreateObject() throws Exception {
        Date dataLowBound = null;
        Date dataUpBound = null;

        // 将分隔符设置为/!a （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        s3Client.putObject( bucketName, keyName, "testcontext18101" );
        expEtags.add( TestTools.getMD5( "testcontext18101".getBytes() ) );
        // 重复上传多次同名对象，检查对象最新LastModified时间在 【倒数第二次上传时间，最后上传完成后时间】范围内
        for ( int i = 0; i < putSameObjTimes - 1; i++ ) {
            String currContent = "newtestcontext18101_" + i;
            s3Client.putObject( bucketName, keyName, currContent );
            expEtags.add( TestTools.getMD5( currContent.getBytes() ) );
        }

        // 上传最后一个对象
        String currContent = "newtestcontext18101_" + ( putSameObjTimes - 1 );
        s3Client.putObject( bucketName, keyName, currContent );
        dataUpBound = new Date();
        expEtags.add( TestTools.getMD5( currContent.getBytes() ) );

        S3Object obj = s3Client.getObject( new GetObjectRequest( bucketName,
                keyName, String.valueOf( putSameObjTimes - 1 ) ) );
        dataLowBound = obj.getObjectMetadata().getLastModified();

        // 此时最后一次上传对象的versionid为putSameObjTimes
        checkResult( dataLowBound, dataUpBound,
                String.valueOf( putSameObjTimes ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                TestTools.LocalFile.removeFile( localPath );
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkResult( Date expDateLowBound, Date expDateUpBound,
            String versionId ) throws Exception {
        // 检查最新上传对象创建时间
        S3Object obj = s3Client.getObject(
                new GetObjectRequest( bucketName, keyName, versionId ) );
        ObjectMetadata metadata = obj.getObjectMetadata();
        Date actCreateDate = metadata.getLastModified();
        if ( actCreateDate.before( expDateLowBound )
                || actCreateDate.after( expDateUpBound ) ) {
            Assert.fail( "create time is different! the actCreateDate is : "
                    + actCreateDate.toString() + ",the expDate is in :[ "
                    + expDateLowBound.toString() + " ~ "
                    + expDateUpBound.toString() + " ]" );
        }

        // 检查桶中对象版本列表信息
        checkObjList();

        // 通过携带delimiter查询对象列表的对外映射场景检测目录表是否生成新目录，对象元数据表和目录表中数据通过连接db手工校验
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< String > commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), expCommPerfix );
        Assert.assertEquals( result.getObjectSummaries().size(), 0 );
    }

    private void checkObjList() {
        List< String > actEtags = new ArrayList<>();
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List< S3VersionSummary > objectVersionList = versionList
                .getVersionSummaries();
        Assert.assertEquals( objectVersionList.size(), putSameObjTimes + 1 );
        for ( S3VersionSummary obj : objectVersionList ) {
            Assert.assertEquals( obj.getBucketName(), bucketName,
                    "bucketName is wrong!" );
            Assert.assertEquals( obj.getKey(), keyName, "keyName is wrong!" );
            actEtags.add( obj.getETag() );
        }
        Collections.sort( expEtags );
        Collections.sort( actEtags );
        Assert.assertEquals( actEtags, expEtags,
                "etag is wrong! , the act etag is :" + actEtags.toString()
                        + ", exp etag is : " + expEtags.toString() );
    }
}
