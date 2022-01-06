package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.*;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description: seqDB-24248:使用deleteObjects接口，关闭版本控制删除对象
 *
 * @author YiPan
 * @Date 2021.6.7
 * @version 1.00
 */
public class DeleteObjects24248 extends S3TestBase {
    private String bucketName = "bucket24248";
    private String keyName = "testkey_24248";
    private String file = "object24248";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;
    private List< String > expDeletekey;
    private List< String > expExistkey;
    private DeleteObjectsRequest deleteRequest;
    private List< DeleteObjectsResult.DeletedObject > deletedObjects;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        expDeletekey = new ArrayList<>();
        expExistkey = new ArrayList<>();
        // 预期被删除对象的key
        for ( int i = 0; i < 10; i++ ) {
            expDeletekey.add( keyName + i );
        }
        // 预期剩余对象的key
        for ( int i = 10; i < 20; i++ ) {
            expExistkey.add( keyName + i );
        }
    }

    @Test
    public void test() throws Exception {
        // 新建桶 未进行任何版本控制操作，不指定版本号删除对象
        String deleteMarkerVersion;
        String deleteversion;
        putObject();
        deleteversion = null;
        deleteMarkerVersion = null;
        deleteRequest = getDeleteRequest( deleteversion );
        deletedObjects = s3Client.deleteObjects( deleteRequest )
                .getDeletedObjects();
        checkDelete( deletedObjects, deleteMarkerVersion, deleteversion );

        // 关闭版本控制,不指定版本号删除对象
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        putObject();
        deleteversion = null;
        deleteMarkerVersion = "null";
        deleteRequest = getDeleteRequest( deleteversion );
        deletedObjects = s3Client.deleteObjects( deleteRequest )
                .getDeletedObjects();
        checkDelete( deletedObjects, deleteMarkerVersion, deleteversion );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.clearBucket( s3Client, bucketName );
            s3Client.shutdown();
        }
    }

    private void putObject() {
        for ( int i = 0; i < 20; i++ ) {
            s3Client.putObject( bucketName, keyName + i, file );
        }
    }

    private DeleteObjectsRequest getDeleteRequest( String version ) {
        List< DeleteObjectsRequest.KeyVersion > deleteKeyVersion = new ArrayList<>();
        for ( int i = 0; i < 10; i++ ) {
            if ( version == null ) {
                deleteKeyVersion.add(
                        new DeleteObjectsRequest.KeyVersion( keyName + i ) );
            } else {
                deleteKeyVersion.add( new DeleteObjectsRequest.KeyVersion(
                        keyName + i, version ) );
            }
        }
        DeleteObjectsRequest request = new DeleteObjectsRequest( bucketName );
        request.setKeys( deleteKeyVersion );
        return request;
    }

    private void checkDelete(
            List< DeleteObjectsResult.DeletedObject > deletedObjects,
            String DeleteMarkerVersionId, String version ) {
        List< String > actDeletekey = new ArrayList<>();
        List< String > actExistkey = new ArrayList<>();
        for ( DeleteObjectsResult.DeletedObject obj : deletedObjects ) {
            actDeletekey.add( obj.getKey() );
            obj.getDeleteMarkerVersionId();
            Assert.assertEquals( obj.getDeleteMarkerVersionId(),
                    DeleteMarkerVersionId );
            Assert.assertEquals( obj.getVersionId(), version );
        }
        List< S3ObjectSummary > objectSummaries = s3Client
                .listObjects( bucketName ).getObjectSummaries();
        for ( S3ObjectSummary s : objectSummaries ) {
            actExistkey.add( s.getKey() );
        }
        Assert.assertEquals( actDeletekey, expDeletekey );
        Assert.assertEquals( actExistkey, expExistkey );
    }
}
