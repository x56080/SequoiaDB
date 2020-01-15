package com.sequoias3.testcommon.s3utils;

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.SkipException;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.ListRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

public class RegionUtils extends S3TestBase {
    private static MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );
    private static String pregix = "S3_";
    private static SimpleDateFormat yearFm = new SimpleDateFormat( "yyyy" );
    private static SimpleDateFormat monthFm = new SimpleDateFormat( "MM" );

    public static void putRegion( Region region ) throws Exception {
        putRegion( region, S3TestBase.s3AccessKeyId );
    }

    public static void putRegion( Region region, String accessKeyId )
            throws Exception {
        TestRest rest = new TestRest( type );
        ResponseEntity< ? > resp;
        try {
            resp = rest.setApi( "/region/?Action=CreateRegion&RegionName=" +
                    region.getName() )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + accessKeyId + "/" )
                    .setRequestBody( region )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            int status = resp.getStatusCodeValue();
            if ( status != 200 ) {
                System.out.println(
                        "put region failed,region = " + region.toString() );
            }
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazon( e );
        }
    }

    public static void clearRegion( String regionName ) throws Exception {
        if ( headRegion( regionName ) ) {
            deleteRegion( regionName );
        }
    }

    public static boolean deleteRegion( String regionName ) throws Exception {
        return deleteRegion( regionName, S3TestBase.s3AccessKeyId );
    }

    public static boolean deleteRegion( String regionName, String accessKeyId )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        boolean isDelete = false;
        try {
            resp = rest.setApi(
                    "/region/?Action=DeleteRegion&RegionName=" + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + accessKeyId + "/" )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            if ( resp.getStatusCodeValue() == 204 ) {
                isDelete = true;
            }
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazon( e );
        }
        return isDelete;
    }

    public static GetRegionResult getRegion( String regionName )
            throws Exception {
        return getRegion( regionName, S3TestBase.s3AccessKeyId );
    }

    public static GetRegionResult getRegion( String regionName,
            String accessKeyId ) throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        GetRegionResult result;
        try {
            resp = rest.setApi(
                    "/region/?Action=GetRegion&RegionName=" + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + accessKeyId + "/" )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            result = stringToObject( xmlBody );
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazon( e );
        }
        return result;
    }

    public static boolean headRegion( String regionName ) throws Exception {
        return headRegion( regionName, S3TestBase.s3AccessKeyId );
    }

    public static boolean headRegion( String regionName, String accessKeyId )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        boolean doesExist = false;
        try {
            resp = rest.setApi(
                    "/region/?Action=HeadRegion&RegionName=" + regionName )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + accessKeyId + "/" )
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

    public static List< String > listRegions() throws Exception {
        return listRegions( S3TestBase.s3AccessKeyId );
    }

    public static List< String > listRegions( String accessKeyId )
            throws Exception {
        TestRest rest = new TestRest();
        ResponseEntity< ? > resp;
        try {
            resp = rest.setApi( "/region/?Action=ListRegions" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + accessKeyId + "/" )
                    .setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
            String xmlBody = resp.getBody().toString();
            XmlMapper xmlMapper = new XmlMapper();
            xmlMapper.setSerializationInclusion( JsonInclude.Include.NON_NULL );
            return xmlMapper.readValue( xmlBody, ListRegionResult.class )
                    .getRegions();
        } catch ( HttpClientErrorException e ) {
            throw httpToAmazon( e );
        }
    }

    private static GetRegionResult stringToObject( String xmlBody )
            throws IOException {
        XmlMapper xmlMapper = new XmlMapper();
        xmlMapper.setSerializationInclusion( JsonInclude.Include.NON_NULL );
        return xmlMapper.readValue( xmlBody, GetRegionResult.class );
    }

    public static AmazonS3Exception httpToAmazon( HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        JSONObject jsonBody = XML.toJSONObject( e.getResponseBodyAsString() );
        JSONObject subjsonBody = jsonBody.getJSONObject( "Error" );
        amazonS3Exception.setErrorCode( subjsonBody.getString( "Code" ) );
        amazonS3Exception.setErrorMessage( subjsonBody.getString( "Message" ) );
        return amazonS3Exception;
    }

    private static AmazonS3Exception httpToAmazonHead(
            HttpClientErrorException e ) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(
                e.getMessage() );
        amazonS3Exception.setStatusCode( e.getStatusCode().value() );
        return amazonS3Exception;
    }

    public static void createCSAndCL( String csName, String[] clNames ) {
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName );
            for ( int i = 0; i < clNames.length; i++ ) {
                cs.createCollection( clNames[ i ] );
            }
        }
    }

    public static void dropCS( String[] csNames ) {
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ) {
            for ( int i = 0; i < csNames.length; i++ ) {
                if ( sdb.isCollectionSpaceExist( csNames[ i ] ) ) {
                    sdb.dropCollectionSpace( csNames[ i ] );
                }
            }
        }
    }

    public static String getDataCSName( String regionName, String shardType,
            Date currTime ) {
        return pregix + regionName + "_DataCS_" +
                getCsClPostfix( shardType, currTime );
    }

    public static String getMetaCSName( String regionName ) {
        return pregix + regionName + "_MetaCS";
    }

    public static String getDataCLName( String shardType, Date currTime ) {
        return pregix + "ObjectData_" + getCsClPostfix( shardType, currTime );
    }

    public static String getCsClPostfix( String shardType, Date currTime ) {
        String currY = yearFm.format( currTime );
        String currM = monthFm.format( currTime );
        String postfix = null;
        if ( shardType.equals( "none" ) ) {
            postfix = "";
        } else if ( shardType.equals( "year" ) ) {
            postfix = currY;
        } else if ( shardType.equals( "quarter" ) ) {
            int quarter = ( int ) Math.ceil( Double.parseDouble( currM ) / 3 );
            postfix = "Q" + quarter;
        } else if ( shardType.equals( "month" ) ) {
            postfix = currM;
        }
        return postfix;
    }

    public static boolean clInCS( String csName, String clName ) {
        Sequoiadb db = null;
        boolean doesCLExist;
        try {
            db = new Sequoiadb( S3TestBase.coordUrl, "", "" );
            doesCLExist = db.getCollectionSpace( csName )
                    .isCollectionExist( clName );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
        return doesCLExist;
    }

    public static boolean doesCSExist( String csName ) {
        Sequoiadb db = null;
        boolean flag;
        try {
            db = new Sequoiadb( S3TestBase.coordUrl, "", "" );
            flag = db.isCollectionSpaceExist( csName );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
        return flag;
    }

    public static void createDomain( String domainName ) {
        Sequoiadb sdb = null;
        try {
            sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" );
            if ( !sdb.isDomainExist( domainName ) ) {
                List< String > groupList = sdb.getReplicaGroupNames();
                groupList.remove( "SYSCatalogGroup" );
                groupList.remove( "SYSCoord" );
                groupList.remove( "SYSSpare" );
                BSONObject option = new BasicBSONObject();
                BSONObject groups = new BasicBSONList();
                if ( groupList.size() < 1 ) {
                    throw new SkipException(
                            "At least one group is required!!! please check " +
                                    "env" );
                }
                for ( int i = 0; i < groupList.size(); i++ ) {
                    groups.put( String.valueOf( i ), groupList.get( i ) );
                }
                option.put( "Groups", groups );
                sdb.createDomain( domainName, option );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    public static void dropDomain( String domainName ) {
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ) {
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        }
    }

    public static void dropCS( String csName ) {
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        }
    }

    public static List< String > listCS( String prefix ) {
        List< String > list = new ArrayList<>();
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        try {
            sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" );
            cursor = sdb.listCollectionSpaces();
            while ( cursor.hasNext() ) {
                BasicBSONObject bson = ( BasicBSONObject ) cursor.getNext();
                if ( bson.getString( "Name" ).startsWith( prefix ) ) {
                    list.add( bson.getString( "Name" ) );
                }
            }
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( sdb != null ) {
                sdb.close();
            }
        }
        return list;
    }

    public static int getRecordNum( String csName, String clName ) {
        Sequoiadb sdb = null;
        int count = 0;
        DBCursor cursor;
        try {
            sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" );
            cursor = sdb.getCollectionSpace( csName ).getCollection( clName )
                    .listLobs();
            while ( cursor.hasNext() ) {
                cursor.getNext();
                count++;
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
        return count;
    }

    public static void checkRegionWithLocation( String regionName,
            String metaLocation, String metaHisLocation, String dataLocation )
            throws Exception {
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getMetaLocation(), metaLocation );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), metaHisLocation );
        Assert.assertEquals( regionInfo.getDataLocation(), dataLocation );
    }

    public static void checkRegionWithShardingType( String regionName,
            String clShardingType, String csShardingType ) throws Exception {
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getDataCLShardingType(),
                clShardingType );
        // get the region infor to take the default value
        Assert.assertEquals( regionInfo.getDataCSShardingType(),
                csShardingType );
        Assert.assertEquals( regionInfo.getMetaDomain(), "" );
        Assert.assertEquals( regionInfo.getDataDomain(), "" );
        Assert.assertEquals( regionInfo.getMetaLocation(), "" );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), "" );
        Assert.assertEquals( regionInfo.getDataLocation(), "" );
    }
}
