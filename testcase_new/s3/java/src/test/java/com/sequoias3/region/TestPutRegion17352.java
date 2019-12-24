package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: PutRegion接口参数校验 testlink-case: seqDB-17352
 *
 * @author wangkexin
 * @Date 2019.01.28
 * @version 1.00
 */
public class TestPutRegion17352 extends S3TestBase {
    private String dataDomain = "dataDomain17352";
    private String metaDomain = "metaDomain17352";
    private String metaCSName = "metaCS17352";
    private String dataCSName = "dataCS17352";
    private String[] metaClNames = { "metaCL17352", "metaHistoryCL17352" };
    private String[] dataClName = { "dataCL17352" };
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "legalRegionNameProvider")
    public Object[][] generateRegionName() {
        return new Object[][] {
                // test a : 范围内取值
                new Object[] { "beijing-1", "specifiedMode", null, null },
                new Object[] { "shanghai", "specifiedMode", null, null },
                // test b : 长度边界值
                new Object[] { ObjectUtils.getRandomString( 3 ),
                        "specifiedMode", null, null },
                new Object[] { ObjectUtils.getRandomString( 20 ),
                        "shardingTypeMode", "year", "quarter" },
                // test c : 包含字母 数字字符[0-9a-zA-Z]
                new Object[] { "01abcdefgABCDEFG", "shardingTypeMode",
                        "quarter", "month" },
                new Object[] { "234hijklmnHIJKLMN", "shardingTypeMode", "month",
                        "year" },
                new Object[] { "567opqrstuOPQRSTU", "specifiedMode", null,
                        null },
                new Object[] { "89vwxyzVWXYZ", "specifiedMode", null, null }, };
    }

    @DataProvider(name = "illegalRegionNameProvider")
    public Object[][] generateIllegalRegionName() {
        return new Object[][] {
                // test a : 超过边界值
                new Object[] { "" },
                new Object[] { ObjectUtils.getRandomString( 2 ) },
                new Object[] { ObjectUtils.getRandomString( 21 ) },
                // test b : 包含特殊字符
                new Object[] { "test!" }, new Object[] { "test_" },
                new Object[] { "test." }, new Object[] { "test*" },
                new Object[] { "test'" }, new Object[] { "test(" },
                new Object[] { "test)" }, new Object[] { "中文" }, };
    }

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.dropDomain( metaDomain );
        RegionUtils.dropDomain( dataDomain );

        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );

        RegionUtils.createDomain( dataDomain );
        RegionUtils.createDomain( metaDomain );
    }

    @Test(dataProvider = "legalRegionNameProvider")
    public void legalRegionName( String regionName, String mode,
            String dataCSShardingType, String dataCLShardingType )
            throws Exception {
        RegionUtils.clearRegion( regionName );
        // test a : specified mode
        if ( mode.equals( "specifiedMode" ) ) {
            Region region = new Region();
            region.withName( regionName )
                    .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                    .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                    .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
            RegionUtils.putRegion( region );

            checkSpecifiedMode( regionName );
        }

        if ( mode.equals( "shardingTypeMode" ) ) {
            Region region = new Region();
            region.withName( regionName )
                    .withDataCSShardingType( dataCSShardingType )
                    .withDataCLShardingType( dataCLShardingType )
                    .withDataDomain( dataDomain ).withMetaDomain( metaDomain );
            RegionUtils.putRegion( region );

            checkShardingTypeMode( regionName, dataCSShardingType,
                    dataCLShardingType );
        }
        RegionUtils.deleteRegion( regionName );
        actSuccessTests.getAndIncrement();
    }

    @Test(dataProvider = "illegalRegionNameProvider")
    public void illegalRegionName( String regionName ) throws Exception {
        try {
            Region region = new Region();
            region.withName( regionName );
            RegionUtils.putRegion( region );
            Assert.fail( "put region with illegal region name should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidRegionName" );
        }
        actSuccessTests.getAndIncrement();
    }

    @Test
    public void illegalParameterName() throws Exception {
        // specified mode
        try {
            Region region = new Region();
            region.withName( "region17352" ).withMetaLocation( "" )
                    .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                    .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
            RegionUtils.putRegion( region );
            Assert.fail( "put region with illegal metaLocation should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidLocation" );
        }

        try {
            Region region = new Region();
            region.withName( "region17352" )
                    .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                    .withMetaHisLocation( "" )
                    .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
            RegionUtils.putRegion( region );
            Assert.fail(
                    "put region with illegal metaHisLocation should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidLocation" );
        }

        try {
            Region region = new Region();
            region.withName( "region17352" )
                    .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                    .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                    .withDataLocation( "" );
            RegionUtils.putRegion( region );
            Assert.fail( "put region with illegal dataLocation should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidLocation" );
        }

        // ShardingType mode
        try {
            Region region = new Region();
            region.withName( "region17352" ).withDataCSShardingType( "day" )
                    .withDataCLShardingType( "month" )
                    .withDataDomain( dataDomain ).withMetaDomain( metaDomain );
            RegionUtils.putRegion( region );
            Assert.fail(
                    "put region with illegal dataCSShardingType should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidShardingType" );
        }

        try {
            Region region = new Region();
            region.withName( "region17352" ).withDataCSShardingType( "year" )
                    .withDataCLShardingType( "day" )
                    .withDataDomain( dataDomain ).withMetaDomain( metaDomain );
            RegionUtils.putRegion( region );
            Assert.fail(
                    "put region with illegal dataCLShardingType should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidShardingType" );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( actSuccessTests.get() == ( generateRegionName().length
                + generateIllegalRegionName().length + 1 ) ) {
            try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "",
                    "" ) ) {
                sdb.dropCollectionSpace( metaCSName );
                sdb.dropCollectionSpace( dataCSName );
                sdb.dropDomain( dataDomain );
                sdb.dropDomain( metaDomain );
            }
        }
    }

    private void checkSpecifiedMode( String regionName ) throws Exception {
        Assert.assertTrue( RegionUtils.headRegion( regionName ) );
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region region = result.getRegion();
        Assert.assertEquals( region.getName(), regionName.toLowerCase() );
        Assert.assertEquals( region.getMetaLocation(),
                metaCSName + "." + metaClNames[ 0 ] );
        Assert.assertEquals( region.getMetaHisLocation(),
                metaCSName + "." + metaClNames[ 1 ] );
        Assert.assertEquals( region.getDataLocation(),
                dataCSName + "." + dataClName[ 0 ] );
    }

    private void checkShardingTypeMode( String regionName,
            String dataCSShardingType, String dataCLShardingType )
            throws Exception {
        Assert.assertTrue( RegionUtils.headRegion( regionName ) );
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region region = result.getRegion();
        Assert.assertEquals( region.getName(), regionName.toLowerCase() );
        Assert.assertEquals( region.getDataCSShardingType(),
                dataCSShardingType );
        Assert.assertEquals( region.getDataCLShardingType(),
                dataCLShardingType );
        Assert.assertEquals( region.getMetaDomain(), metaDomain );
        Assert.assertEquals( region.getDataDomain(), dataDomain );
    }
}
