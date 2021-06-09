package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.*;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
;
import java.util.*;

/**
 * @Description: seqDB-24255:使用deleteObjects接口，开启版本控制删除对象
 *
 * @author YiPan
 * @Date 2021.6.7
 * @version 1.00
 */
public class DeleteObjects24255 extends S3TestBase {
    private String bucketName = "bucket24255";
    private String keyName = "testkey_24255";
    private String file = "object24255";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;
    private List< String > allObjectKeys;
    private DeleteObjectsRequest deleteRequest;
    private List< DeleteObjectsResult.DeletedObject > deletedObjects;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        allObjectKeys = new ArrayList<>();
        for ( int i = 0; i < 10; i++ ) {
            allObjectKeys.add( keyName + i );
        }
    }

    @Test
    public void test() throws Exception {
        String deleteMarkerVersion ;
        String deleteVersion ;
        String historyVersion ;
        String LastestVersion ;

        // 不指定版本号删除对象
        CommLib.deleteAllObjectVersions( s3Client, bucketName );
        putObject( 5 );
        deleteMarkerVersion = "5";
        deleteVersion = null;
        historyVersion = "4";
        deleteRequest = getDeleteRequest( deleteVersion );
        deletedObjects = s3Client.deleteObjects( deleteRequest )
                .getDeletedObjects();
        checkDeleteInfo( deletedObjects, allObjectKeys, deleteMarkerVersion, deleteVersion );
        checkExistObjectAndVersion(new ArrayList< String >(),null,null);
        checkObjectVersion(historyVersion);
        
        // 删除版本为"4"的对象
        CommLib.deleteAllObjectVersions( s3Client, bucketName );
        putObject( 5 );
        deleteVersion = "4";
        LastestVersion = "3";
        deleteMarkerVersion = null;
        deleteRequest = getDeleteRequest( deleteVersion );
        deletedObjects = s3Client.deleteObjects( deleteRequest )
                .getDeletedObjects();
        checkDeleteInfo( deletedObjects, allObjectKeys, deleteMarkerVersion, deleteVersion );
        checkExistObjectAndVersion(allObjectKeys, LastestVersion, deleteVersion );
        // 删除版本为"1"的对象
        deleteVersion = "1";
        LastestVersion = "3";
        deleteMarkerVersion = null;
        deleteRequest = getDeleteRequest( deleteVersion );
        deletedObjects = s3Client.deleteObjects( deleteRequest )
                .getDeletedObjects();
        checkDeleteInfo( deletedObjects, allObjectKeys, deleteMarkerVersion, deleteVersion );
        checkExistObjectAndVersion(allObjectKeys, LastestVersion, deleteVersion );
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.clearBucket( s3Client, bucketName );
            s3Client.shutdown();
        }
    }

    private void putObject( int VersionNum ) {
        for ( int j = 0; j < VersionNum; j++ ) {
            for ( int i = 0; i < 10; i++ ) {
                s3Client.putObject( bucketName, keyName + i, file );
            }
        }
    }

    private DeleteObjectsRequest getDeleteRequest( String version ) {
        List< DeleteObjectsRequest.KeyVersion > deleteVersion = new ArrayList<>();
        for ( int i = 0; i < 10; i++ ) {
            if ( version == null ) {
                deleteVersion.add(
                        new DeleteObjectsRequest.KeyVersion( keyName + i ) );
            } else {
                deleteVersion.add( new DeleteObjectsRequest.KeyVersion(
                        keyName + i, version ) );
            }
        }
        DeleteObjectsRequest request = new DeleteObjectsRequest( bucketName );
        request.setKeys( deleteVersion );
        return request;
    }

    private void checkDeleteInfo(
            List< DeleteObjectsResult.DeletedObject > deletedObjects,
            List< String > expDeleteObjectskey, String DeleteMarkerVersionId,
            String version ) {
        List< String > actDeletekey = new ArrayList<>();
        for ( DeleteObjectsResult.DeletedObject obj : deletedObjects ) {
            actDeletekey.add( obj.getKey() );
            obj.getDeleteMarkerVersionId();
            Assert.assertEquals( obj.getDeleteMarkerVersionId(),
                    DeleteMarkerVersionId );
            Assert.assertEquals( obj.getVersionId(), version );
        }
        Assert.assertEquals( actDeletekey, expDeleteObjectskey );
    }

    private void checkExistObjectAndVersion(List< String > expExistkey, String version,
            String deleteVersion ) {
        List< String > actExistkey = new ArrayList<>();
        List< S3ObjectSummary > objectSummaries = s3Client
                .listObjects( bucketName ).getObjectSummaries();
        for ( S3ObjectSummary objectSummary : objectSummaries ) {
            actExistkey.add( objectSummary.getKey() );
            S3Object object = s3Client.getObject( bucketName,
                    objectSummary.getKey() );
            // 校验当前版本
            Assert.assertEquals( object.getObjectMetadata().getVersionId(),
                    version );
            // 校验已删除版本
            GetObjectRequest getObjectRequest = new GetObjectRequest(
                    bucketName, objectSummary.getKey(), deleteVersion );
            try {
                s3Client.getObject( getObjectRequest );
                Assert.fail("except fail but success");
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(), "NoSuchVersion" );
            }
        }
        Collections.sort( actExistkey );
        Collections.sort( expExistkey );
        Assert.assertEquals( actExistkey, expExistkey );

    }
    
    private void checkObjectVersion(String historyVersion){
        for(String key:allObjectKeys){
            GetObjectRequest getObjectRequest = new GetObjectRequest(
                    bucketName, key, historyVersion );
                s3Client.getObject( getObjectRequest );
        }
    }
}
