package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Owner;
import com.amazonaws.services.s3.model.Permission;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-19450:无桶访问权限的用户配置桶acl
 * @Author huangxiaoni
 * @Date 2019.09.24
 */

public class SetBucketAcl19450 extends S3TestBase {
    private boolean runSuccess = false;
    private String tcId = "19450";
    private AmazonS3 adminS3 = null;
    private AmazonS3 userS3 = null;
    private String userName = "user" + tcId;
    private String userType = UserCommDefind.normal;
    private String bucketName = "bucket" + tcId;

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLib.buildS3Client();
        CommLib.clearBucket( adminS3, bucketName );
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, userType );
        userS3 = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
    }

    @Test
    private void test() throws Exception {
        adminS3.createBucket( new CreateBucketRequest( bucketName ) );
        AccessControlList adminAcl = adminS3.getBucketAcl( bucketName );
        // check owner
        Owner adminOwner = adminS3.getS3AccountOwner();
        Assert.assertEquals( adminAcl.getOwner(), adminOwner );
        // check grant
        Grant grant = new Grant( new CanonicalGrantee( adminOwner.getId() ),
                Permission.FullControl );
        PrivilegeUtils.checkSetBucketAclResult( adminS3, bucketName, grant );
        // check isPrivate from sdb
        checkIsPrivate();

        // not bucket owner set bucket's acl
        try {
            userS3.setBucketAcl( bucketName, CannedAccessControlList.Private );
            Assert.fail( "expect fail but success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( adminS3, bucketName );
                CommLib.clearUser( userName );
            }
        } finally {
            adminS3.shutdown();
            userS3.shutdown();
        }
    }

    private void checkIsPrivate() {
        try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" ) ) {
            DBCollection cl = sdb.getCollectionSpace( "S3_SYS_Meta" )
                    .getCollection( "S3_Bucket" );
            DBCursor cursor = cl
                    .query( new BasicBSONObject( "Name", bucketName ), null,
                            null, null );
            boolean IsPrivate = ( boolean ) cursor.getNext().get( "IsPrivate" );
            Assert.assertTrue( IsPrivate );
            cursor.close();
        }
    }
}