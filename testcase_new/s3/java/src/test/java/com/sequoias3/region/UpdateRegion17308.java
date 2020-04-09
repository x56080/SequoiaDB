package com.sequoias3.region;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.Calendar;
import java.util.Date;

/**
 * @Description: seqDB-17308 ::
 *               更新区域，配置DataCSShardingType/DataCLShardingType/Domain
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17308 extends S3TestBase {
    private String regionName = "region17308";
    private String bucketName = "bucket17308";
    private String objectName = "object17308";
    private String dataCLShardingType = "month";
    private String upDataCLShardingType = "year";
    private String dataCSShardingType = "year";
    private String upDataCSShardingType = "month";
    private String domainName = "domain17308";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 200;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + ( fileSize )
                + ".txt";
        updatePath = localPath + File.separator + "localFile_"
                + ( fileSize + 1024 ) + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, fileSize );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
        RegionUtils.createDomain( domainName );
    }

    @Test
    private void test() throws Exception {
        // create region
        Region region = new Region();
        region.withDataCSShardingType( dataCSShardingType )
                .withDataCLShardingType( dataCLShardingType )
                .withMetaDomain( domainName ).withDataDomain( domainName )
                .withName( regionName );
        RegionUtils.putRegion( region );

        // create bucket and object
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
        s3Client.putObject( bucketName, objectName, new File( filePath ) );

        region.withDataCSShardingType( upDataCSShardingType )
                .withDataCLShardingType( upDataCLShardingType )
                .withMetaDomain( domainName ).withDataDomain( domainName )
                .withName( regionName );
        RegionUtils.putRegion( region );
        GetRegionResult result = RegionUtils.getRegion( regionName );
        checkGetRegionResult( result, region );

        // create object again
        s3Client.putObject( bucketName, objectName, new File( updatePath ) );

        // get cs and cl
        Date date = Calendar.getInstance().getTime();
        String csName1 = RegionUtils.getDataCSName( regionName,
                dataCSShardingType, date ) + "_1";
        String csName2 = RegionUtils.getDataCSName( regionName,
                upDataCSShardingType, date ) + "_1";
        String clName1 = RegionUtils.getDataCLName( dataCLShardingType, date );
        String clName2 = RegionUtils.getDataCLName( upDataCLShardingType,
                date );

        // count the number of record
        int count1 = RegionUtils.getRecordNum( csName1, clName1 );
        int count2 = RegionUtils.getRecordNum( csName2, clName2 );
        Assert.assertEquals( count1, 0, "csName1 = " + csName1 + ",clName1 = "
                + clName1 + ",objectName = " + objectName );
        Assert.assertEquals( count2, 1, "csName2 = " + csName2 + ",clName2 = "
                + clName2 + ",objectName = " + objectName );

        // get object for check
        S3Object s3Object = s3Client.getObject( bucketName, objectName );
        checkObjectMetaAndData( s3Object, updatePath );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                RegionUtils.deleteRegion( regionName );
                RegionUtils.dropDomain( domainName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkObjectMetaAndData( S3Object object, String filePath )
            throws Exception {
        ObjectMetadata metadata = object.getObjectMetadata();
        Assert.assertEquals( metadata.getVersionId(), "null" );
        Assert.assertEquals( metadata.getETag(), TestTools.getMD5( filePath ) );
        String downloadPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( object.getObjectContent(), downloadPath );
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( filePath ), "filePath = " + filePath );
    }

    private void checkGetRegionResult( GetRegionResult result,
            Region expRegion ) {
        Region actRegion = result.getRegion();
        Assert.assertEquals( actRegion.getDataCSShardingType(),
                expRegion.getDataCSShardingType() );
        Assert.assertEquals( actRegion.getDataCLShardingType(),
                expRegion.getDataCLShardingType() );
    }
}
