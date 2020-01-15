package com.sequoias3.region;

import java.io.File;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;

/**
 * @Description: seqDB-17304 :: 更新区域，配置DataCSShardingType
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17304 extends S3TestBase {
    private String regionName = "region17304";
    private String bucketName = "bucket17304";
    private String objectName = "object17304";
    private String dataCSShardingType = "year";
    private String upDataCSShardingType = "month";
    private String dataCLShardingType = "month";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 200;
    private File localPath = null;
    private List<String> filePathList = new ArrayList<String>();
    private int fileNum = 2;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        String filePath = null;
        for ( int i = 0; i < fileNum; i++ ) {
            filePath =
                    localPath + File.separator + "localFile_" + ( fileSize + i )
                            + ".txt";
            TestTools.LocalFile.createFile( filePath, fileSize + i );
            filePathList.add( filePath );
        }
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    private void test() throws Exception {
        // create region
        Region region = new Region();
        region.withDataCSShardingType( dataCSShardingType )
                .withDataCLShardingType( dataCLShardingType )
                .withName( regionName );
        RegionUtils.putRegion( region );

        // create bucket and object
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
        String objectName1 = objectName + "_" + 0;
        s3Client.putObject( bucketName, objectName1,
                new File( filePathList.get( 0 ) ) );

        // change DataCSShardingType:year to month
        region.withDataCSShardingType( upDataCSShardingType )
                .withDataCLShardingType( dataCLShardingType )
                .withName( regionName );
        RegionUtils.putRegion( region );
        GetRegionResult result = RegionUtils.getRegion( regionName );
        checkGetRegionResult( result, region );

        // create object
        String objectName2 = objectName + "_" + 1;
        s3Client.putObject( bucketName, objectName2,
                new File( filePathList.get( 1 ) ) );

        // get cs and cl
        Date date = Calendar.getInstance().getTime();
        String csName1 = RegionUtils
                .getDataCSName( regionName, dataCSShardingType, date ) + "_1";
        String csName2 = RegionUtils
                .getDataCSName( regionName, upDataCSShardingType, date ) + "_1";
        String clName = RegionUtils.getDataCLName( "month", date );

        // count the number of record
        int count1 = RegionUtils.getRecordNum( csName1, clName );
        int count2 = RegionUtils.getRecordNum( csName2, clName );
        Assert.assertEquals( count1, 1,
                "csName1 = " + csName1 + ",clName1 = " + clName
                        + ",objectName = " + objectName1 );
        Assert.assertEquals( count2, 1,
                "csName2 = " + csName2 + ",clName1 = " + clName
                        + ",objectName = " + objectName2 );

        // get object for check
        for ( int i = 0; i < fileNum; i++ ) {
            S3Object s3Object = s3Client
                    .getObject( bucketName, objectName + "_" + i );
            checkObjectMetaAndData( s3Object, filePathList.get( i ) );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                RegionUtils.deleteRegion( regionName );
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
        String downloadPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( object.getObjectContent(), downloadPath );
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( filePath ), "filePath = " + filePath );
    }

    private void checkGetRegionResult( GetRegionResult result,
            Region expRegion ) {
        Region actRegion = result.getRegion();
        Assert.assertEquals( actRegion.getDataCSShardingType(),
                expRegion.getDataCLShardingType() );
        Assert.assertEquals( actRegion.getDataCLShardingType(),
                expRegion.getDataCLShardingType() );
    }
}
