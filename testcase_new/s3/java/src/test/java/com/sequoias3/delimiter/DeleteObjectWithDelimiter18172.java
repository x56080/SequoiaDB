package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.List;

/**
 * test content: 开启版本控制，不带versionId删除对象 testlink-case: seqDB-18172
 *
 * @author wangkexin
 * @Date 2019.04.29
 * @version 1.00
 */
public class DeleteObjectWithDelimiter18172 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18172";
    private String key = "&&aa&%test中文&object18172";
    private String delimiter = "%";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 300;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        s3Client.putObject( bucketName, key, new File( filePath ) );
    }

    @Test
    public void testDeleteObject() throws Exception {
        s3Client.deleteObject( bucketName, key );
        checkDeleteObjectResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client, bucketName,
                        key );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDeleteObjectResult() throws Exception {
        // 检查删除结果，查看最新元数据表中对象记录已不存在，新增一条对象的删除标记，历史元数据表中新增删除对象的记录，对象对应目录仍存在
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertFalse( isExistObject, "the object should not exist!" );

        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        Assert.assertEquals( versionList.getCommonPrefixes().size(), 0 );

        // 检查删除标记
        List< S3VersionSummary > versions = versionList.getVersionSummaries();
        for ( S3VersionSummary version : versions ) {
            if ( version.getVersionId().equals( "0" ) ) {
                Assert.assertEquals( version.isDeleteMarker(), false );
            } else {
                Assert.assertEquals( version.isDeleteMarker(), true );
            }
            Assert.assertEquals( version.getKey(), key );
        }

        // check the object of version "0"
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key, "0" );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}
