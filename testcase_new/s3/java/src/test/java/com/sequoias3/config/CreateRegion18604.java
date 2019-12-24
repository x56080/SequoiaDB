package com.sequoias3.config;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.util.DateUtils;
import com.sequoias3.region.GetRegionResult;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
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

import java.util.ArrayList;
import java.util.List;

/**
 * test content: 开启鉴权，使用普通用户执行区域管理操作 testlink-case: seqDB-18604
 *
 * @author wangkexin
 * @Date 2019.06.24
 * @version 1.00
 */
public class CreateRegion18604 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private boolean runSuccess = false;
    private String regionName = "region18604";
    private String[] csNames = { "metaCS18604", "dataCS18604" };
    private String[] metaclNames = { "metaCL18604", "metaHistroyCL18604" };
    private String[] dataclNames = { "dataCL18604" };
    private String[] domainNames = { "domain18604A", "domain18604B" };
    private String userName = "normaluser18604";
    private String roleName = "normal";
    private String[] accessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    private void testCreateRegionV2() throws Exception {
        runSuccess = false;
        String authorizationV2 = "AWS " + accessKeys[ 0 ] + ":signature";

        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );
        // create region
        Region region = new Region();
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        region.withMetaLocation( metaLocation ).withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation ).withName( regionName );

        try {
            putRegion( region, authorizationV2 );
            Assert.fail( "create region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        try {
            headRegion( regionName, authorizationV2 );
            Assert.fail( "head region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 403 );
        }

        try {
            getRegion( regionName, authorizationV2 );
            Assert.fail( "get region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        // 所有用户都可查询区域列表
        List<String> regions = listRegions( authorizationV2 );
        Assert.assertFalse( regions.contains( regionName ) );

        try {
            deleteRegion( regionName, authorizationV2 );
            Assert.fail( "delete region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }
        runSuccess = true;
    }

    @Test
    private void testCreateRegionV4() throws Exception {
        runSuccess = false;
        String authorizationV4 =
                UserCommDefind.authValPre + accessKeys[ 0 ] + "/";

        for ( String domainName : domainNames ) {
            RegionUtils.createDomain( domainName );
        }
        // create region
        Region region = new Region();
        region.withDataCSShardingType( "year" )
                .withDataCLShardingType( "month" )
                .withDataDomain( domainNames[ 0 ] )
                .withMetaDomain( domainNames[ 1 ] ).withName( regionName );

        try {
            putRegion( region, authorizationV4 );
            Assert.fail( "create region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        try {
            headRegion( regionName, authorizationV4 );
            Assert.fail( "head region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 403 );
        }

        try {
            getRegion( regionName, authorizationV4 );
            Assert.fail( "get region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        // 所有用户都可查询区域列表
        List<String> regions = listRegions( authorizationV4 );
        Assert.assertFalse( regions.contains( regionName ) );

        try {
            deleteRegion( regionName, authorizationV4 );
            Assert.fail( "delete region " + regionName
                    + " by normal user should be failed!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.dropCS( csNames );
            UserUtils.deleteUser( userName );
        }
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
