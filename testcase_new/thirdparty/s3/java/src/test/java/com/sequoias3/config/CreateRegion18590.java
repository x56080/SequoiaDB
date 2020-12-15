package com.sequoias3.config;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.SequoiaS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.ListRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

/**
 * @Description: testlink-case: seqDB-18590:开启鉴权，执行区域管理操作
 * @author wangkexin
 * @Date 2019.06.24
 * @version 1.00
 */
public class CreateRegion18590 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String regionName1 = "region18590v2";
    private String regionName2 = "region18590v4";
    private String bucketName = "bucket18590";
    private String keyName = "key18590";
    private String[] csNames = { "metaCS18590", "dataCS18590" };
    private String[] metaclNames = { "metaCL18590", "metaHistroyCL18590" };
    private String[] dataclNames = { "dataCL18590" };
    private String[] domainNames = { "domain18590A", "domain18590B" };
    private String authorizationV2 = "AWS " + UserUtils.accessKeyId
            + ":signature";
    private String authorizationV4 = UserCommDefind.authValPre
            + UserUtils.accessKeyId + "/";
    private int fileSize = 1024 * 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

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
        CommLib.clearBucket( s3Client, bucketName );
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName1 );
        RegionUtils.clearRegion( regionClient, regionName2 );
    }

    @Test
    private void testCreateRegionV2() throws Exception {
        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );
        // create region
        Region region = new Region();
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        region.withMetaLocation( metaLocation ).withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation ).withName( regionName1 );
        putRegion( region, authorizationV2 );
        // get region and check region info
        checkRegionV2( metaLocation, metaHisLocation, dataLocation,
                authorizationV2 );
        // create object on region
        createObjectAndCheckResult( regionName1 );

        // delete region and clear environment
        CommLib.clearBucket( s3Client, bucketName );
        deleteRegion( regionName1, authorizationV2 );
        actSuccessTests.getAndIncrement();
    }

    @Test
    private void testCreateRegionV4() throws Exception {
        for ( String domainName : domainNames ) {
            RegionUtils.createDomain( domainName );
        }
        // create region
        Region region = new Region();
        region.withDataCSShardingType( "year" )
                .withDataCLShardingType( "month" )
                .withDataDomain( domainNames[ 0 ] )
                .withMetaDomain( domainNames[ 1 ] ).withName( regionName2 );
        putRegion( region, authorizationV4 );

        // get region and check region info
        checkGetResultV4( region, authorizationV4 );
        // create object on region
        createObjectAndCheckResult( regionName2 );

        CommLib.clearBucket( s3Client, bucketName );
        deleteRegion( regionName2, authorizationV4 );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == 2 ) {
                RegionUtils.dropCS( csNames );
                for ( String domainName : domainNames ) {
                    RegionUtils.dropDomain( domainName );
                }
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( regionClient != null ) {
                regionClient.shutdown();
            }
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkRegionV2( String metaLocation, String metaHisLocation,
            String dataLocation, String authorization ) throws Exception {
        boolean headRegion = headRegion( regionName1, authorization );
        Assert.assertTrue( headRegion, "region should exist." );

        GetRegionResult result = getRegion( regionName1, authorization );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getMetaLocation(), metaLocation );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), metaHisLocation );
        Assert.assertEquals( regionInfo.getDataLocation(), dataLocation );

        List< String > regions = listRegions( authorization );
        Assert.assertTrue( regions.contains( regionName1 ) );
    }

    private void checkGetResultV4( Region expRegion, String authorization )
            throws Exception {
        boolean headRegion = headRegion( regionName2, authorization );
        Assert.assertTrue( headRegion, "region should exist." );

        GetRegionResult result = getRegion( regionName2, authorization );
        Region region = result.getRegion();
        Assert.assertEquals( region.getName(), expRegion.getName() );
        Assert.assertEquals( region.getMetaDomain(),
                expRegion.getMetaDomain() );
        Assert.assertEquals( region.getDataDomain(),
                expRegion.getDataDomain() );
        Assert.assertEquals( region.getDataCSShardingType(),
                expRegion.getDataCSShardingType() );
        Assert.assertEquals( region.getDataCLShardingType(),
                expRegion.getDataCLShardingType() );

        List< String > regions = listRegions( authorization );
        Assert.assertTrue( regions.contains( regionName2 ) );
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult( String regionName )
            throws Exception {
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void putRegion( Region region, String authorization )
            throws Exception {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( "/region/?Action=CreateRegion&RegionName="
                    + region.getName() )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization )
                    .setRequestBody( region )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private boolean deleteRegion( String regionName, String authorization )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        boolean isDelete = false;
        try {
            resp = rest
                    .setApi( "/region/?Action=DeleteRegion&RegionName="
                            + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            if ( resp.getStatusCodeValue() == 204 ) {
                isDelete = true;
            }
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return isDelete;
    }

    private GetRegionResult getRegion( String regionName, String authorization )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        GetRegionResult result;
        try {
            resp = rest
                    .setApi( "/region/?Action=GetRegion&RegionName="
                            + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            result = stringToObject( xmlBody );
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return result;
    }

    private boolean headRegion( String regionName, String authorization )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        boolean doesExist = false;
        try {
            resp = rest
                    .setApi( "/region/?Action=HeadRegion&RegionName="
                            + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            if ( resp.getStatusCodeValue() == 200 ) {
                doesExist = true;
            }
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode().value() != 404 ) {
                throw httpToAmazonHead( e );
            }
        }
        return doesExist;
    }

    private List< String > listRegions( String authorization )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        try {
            resp = rest.setApi( "/region/?Action=ListRegions" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            XmlMapper xmlMapper = new XmlMapper();
            xmlMapper.setSerializationInclusion( JsonInclude.Include.NON_NULL );
            return xmlMapper.readValue( xmlBody, ListRegionResult.class )
                    .getRegions();
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private GetRegionResult stringToObject( String xmlBody )
            throws IOException {
        XmlMapper xmlMapper = new XmlMapper();
        xmlMapper.setSerializationInclusion( JsonInclude.Include.NON_NULL );
        return xmlMapper.readValue( xmlBody, GetRegionResult.class );
    }

    private AmazonS3Exception httpToAmazonHead( HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        return amazonS3Exception;
    }
}
