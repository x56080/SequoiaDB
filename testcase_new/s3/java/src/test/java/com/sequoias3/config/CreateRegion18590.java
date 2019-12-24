package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.util.DateUtils;
import com.sequoias3.region.GetRegionResult;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 开启鉴权，执行区域管理操作 testlink-case: seqDB-18590
 *
 * @author wangkexin
 * @Date 2019.06.24
 * @version 1.00
 */
public class CreateRegion18590 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private boolean runSuccess = false;
    private String regionName1 = "region18590v2";
    private String regionName2 = "region18590v4";
    private String bucketName = "bucket18590";
    private String keyName = "key18590";
    private String[] csNames = { "metaCS18590", "dataCS18590" };
    private String[] metaclNames = { "metaCL18590", "metaHistroyCL18590" };
    private String[] dataclNames = { "dataCL18590" };
    private String[] domainNames = { "domain18590A", "domain18590B" };
    private String authorizationV2 =
            "AWS " + UserUtils.accessKeyId + ":signature";
    private String authorizationV4 =
            UserCommDefind.authValPre + UserUtils.accessKeyId + "/";
    private int fileSize = 1024 * 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName1 );
        RegionUtils.clearRegion( regionName2 );
    }

    @Test
    private void testCreateRegionV2() throws Exception {
        runSuccess = false;
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
        runSuccess = true;
    }

    @Test
    private void testCreateRegionV4() throws Exception {
        runSuccess = false;
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
        checkGetResultV4( authorizationV4 );
        // create object on region
        createObjectAndCheckResult( regionName2 );

        CommLib.clearBucket( s3Client, bucketName );
        deleteRegion( regionName2, authorizationV4 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                RegionUtils.dropCS( csNames );
                for ( String domainName : domainNames ) {
                    RegionUtils.dropDomain( domainName );
                }
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
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

        List<String> regions = listRegions( authorization );
        Assert.assertTrue( regions.contains( regionName1 ) );
    }

    private void checkGetResultV4( String authorization ) throws Exception {
        boolean headRegion = headRegion( regionName2, authorization );
        Assert.assertTrue( headRegion, "region should exist." );

        GetRegionResult result = getRegion( regionName2, authorization );
        // expResult
        Region expRegion = new Region().withDataCSShardingType( "year" )
                .withDataCLShardingType( "month" )
                .withDataDomain( domainNames[ 0 ] )
                .withMetaDomain( domainNames[ 1 ] ).withName( regionName2 )
                .withDataLocation( "" ).withMetaHisLocation( "" )
                .withMetaLocation( "" );

        Region region = result.getRegion();
        Assert.assertEquals( region.toString(), expRegion.toString(),
                "region = " + region.toString() + ",expRegion = " + expRegion
                        .toString() );

        List<String> regions = listRegions( authorization );
        Assert.assertTrue( regions.contains( regionName2 ) );
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult( String regionName )
            throws Exception {
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void putRegion( Region region, String authorization )
            throws Exception {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( "/region/?Action=CreateRegion&RegionName=" + region
                    .getName() )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestBody( region )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }

    private boolean deleteRegion( String regionName, String authorization )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity<?> resp;
        boolean isDelete = false;
        try {
            resp = rest.setApi(
                    "/region/?Action=DeleteRegion&RegionName=" + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.POST )
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
        ResponseEntity<?> resp;
        GetRegionResult result;
        try {
            resp = rest.setApi(
                    "/region/?Action=GetRegion&RegionName=" + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.POST )
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
        ResponseEntity<?> resp;
        boolean doesExist = false;
        try {
            resp = rest.setApi(
                    "/region/?Action=HeadRegion&RegionName=" + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.POST )
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

    private List<String> listRegions( String authorization ) throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity<?> resp;
        List<String> listResult;
        try {
            resp = rest.setApi( "/region/?Action=ListRegions" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            JSONObject jsonBody = XML.toJSONObject( xmlBody );
            JSONObject regions = jsonBody
                    .getJSONObject( "ListAllRegionsResult" );
            Object object = regions.get( "Region" );
            listResult = new ArrayList<>();
            if ( object instanceof JSONArray ) {
                JSONArray array = ( JSONArray ) object;
                for ( int i = 0; i < array.length(); i++ ) {
                    listResult.add( array.getString( i ) );
                }
            } else {
                listResult.add( object.toString() );
            }
        } catch ( HttpClientErrorException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
        return listResult;
    }

    private GetRegionResult stringToObject( String xmlBody ) {
        JSONObject jsonBody = XML.toJSONObject( xmlBody );
        JSONObject subjsonBody = jsonBody
                .getJSONObject( "RegionConfiguration" );
        Region region = new Region();
        region.withName( subjsonBody.getString( "Name" ) );
        region.withDataCSShardingType(
                subjsonBody.getString( "DataCSShardingType" ) );
        region.withDataCLShardingType(
                subjsonBody.getString( "DataCLShardingType" ) );
        region.withDataDomain( subjsonBody.getString( "DataDomain" ) );
        region.withMetaDomain( subjsonBody.getString( "MetaDomain" ) );
        region.withDataLocation( subjsonBody.getString( "DataLocation" ) );
        region.withMetaLocation( subjsonBody.getString( "MetaLocation" ) );
        region.withMetaHisLocation(
                subjsonBody.getString( "MetaHisLocation" ) );
        GetRegionResult result = new GetRegionResult( region );
        List<Bucket> buckets = new ArrayList<>();
        Object objects = subjsonBody.get( "Buckets" );
        if ( objects instanceof JSONObject ) {
            JSONObject jsonObject = ( JSONObject ) objects;
            Object jsonObjectBucket = jsonObject.get( "Bucket" );
            if ( jsonObjectBucket instanceof JSONArray ) {
                JSONArray jsonArray = ( JSONArray ) jsonObjectBucket;
                for ( int i = 0; i < jsonArray.length(); i++ ) {
                    Bucket bucket = new Bucket();
                    JSONObject subjsonObject = jsonArray.getJSONObject( i );
                    bucket.setName( subjsonObject.getString( "Name" ) );
                    bucket.setCreationDate( DateUtils.parseISO8601Date(
                            subjsonObject.getString( "CreationDate" ) ) );
                    buckets.add( bucket );
                }
            } else {
                JSONObject json = ( JSONObject ) jsonObjectBucket;
                Bucket bucket = new Bucket();
                bucket.setName( json.getString( "Name" ) );
                bucket.setCreationDate( DateUtils
                        .parseISO8601Date( json.getString( "CreationDate" ) ) );
                buckets.add( bucket );
            }
        }
        result.setBuckets( buckets );
        return result;
    }

    private AmazonS3Exception httpToAmazonHead( HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        return amazonS3Exception;
    }
}
