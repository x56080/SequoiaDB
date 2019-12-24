package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.EmailAddressGrantee;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Grantee;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-19454: 配置对象acl，被授权人包含非owner的用户
 * @Author wangkexin
 * @Date 2019.09.23
 */
public class SetObjectAcl19454 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private int expRunSuccessNum = 13;
    private String bucketName = "bucket19454";
    private String keyName = "key19454";
    private String userName = "user19454";
    private String roleName = "normal";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String ownerId;
    private String userId;

    @DataProvider(name = "aclProvider")
    private Object[][] generateAclProvider() {
        // parameter : CannedAccessControlList acl, Grant[] expGrant
        return new Object[][] {
                // set bucket acl : public-read
                new Object[] { CannedAccessControlList.PublicRead, new Grant[] {
                        new Grant( new CanonicalGrantee( ownerId ),
                                Permission.FullControl ),
                        new Grant( GroupGrantee.AllUsers, Permission.Read ) } },
                // set bucket acl : public-read-write
                new Object[] { CannedAccessControlList.PublicReadWrite,
                        new Grant[] {
                                new Grant( new CanonicalGrantee( ownerId ),
                                        Permission.FullControl ),
                                new Grant( GroupGrantee.AllUsers,
                                        Permission.Read ),
                                new Grant( GroupGrantee.AllUsers,
                                        Permission.Write ) } },
                // set bucket acl : authenticated-read
                new Object[] { CannedAccessControlList.AuthenticatedRead,
                        new Grant[] {
                                new Grant( new CanonicalGrantee( ownerId ),
                                        Permission.FullControl ),
                                new Grant( GroupGrantee.AuthenticatedUsers,
                                        Permission.Read ) } } };
    }

    @DataProvider(name = "grantProvider")
    private Object[][] generateGrantProvider() {
        // parameter : grantee
        return new Object[][] {
                // set bucket acl grantee: id of non-bucket owner
                new Object[] { new CanonicalGrantee( userId ) },
                // set bucket acl grantee: uri(a perdefined s3 group)
                new Object[] { GroupGrantee.AllUsers },
                new Object[] { GroupGrantee.AuthenticatedUsers },
                new Object[] { GroupGrantee.LogDelivery },
                // set bucket acl grantee: emailAddress
                new Object[] { new EmailAddressGrantee(
                        "test19454 email address" ) } };
    }

    @BeforeClass
    private void setUp() throws IOException {
        // create a user
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        userS3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        userId = userS3Client.getS3AccountOwner().getId();

        ownerS3Client = CommLib.buildS3Client();
        ownerId = ownerS3Client.getS3AccountOwner().getId();
        CommLib.clearBucket( ownerS3Client, bucketName );
        ownerS3Client.createBucket( new CreateBucketRequest( bucketName ) );
        putObjectBeforeTest();

    }

    @Test(dataProvider = "aclProvider")
    private void testSetObjectAcl1( CannedAccessControlList acl,
            Grant[] expGrant ) throws Exception {
        // set object acl using standard acl mode
        ownerS3Client.setObjectAcl( bucketName, keyName, acl );
        PrivilegeUtils
                .checkSetObjectAclResult( ownerS3Client, bucketName, keyName,
                        expGrant );
        getObjectByOtherUser();
        actSuccessTests.getAndIncrement();
    }

    @Test(dataProvider = "grantProvider")
    private void testSetObjectAcl2( Grantee grantee ) throws Exception {
        // set object acl with x-amz-grant-* in the request header
        for ( Permission permission : Permission.values() ) {
            Grant expGrant = new Grant( grantee, permission );
            PrivilegeUtils
                    .setObjectAclByHeader( s3AccessKeyId, bucketName, keyName,
                            expGrant );
            PrivilegeUtils.checkSetObjectAclResult( ownerS3Client, bucketName,
                    keyName, expGrant );
            getObjectByOtherUser();
        }
        actSuccessTests.getAndIncrement();
    }

    @Test(dataProvider = "grantProvider")
    private void testSetObjectAcl3( Grantee grantee ) throws Exception {
        // set object acl with access control list in request body
        for ( Permission permission : Permission.values() ) {
            Grant expGrant = new Grant( grantee, permission );
            PrivilegeUtils
                    .setObjectAclByBody( ownerS3Client, bucketName, keyName,
                            expGrant );
            PrivilegeUtils.checkSetObjectAclResult( ownerS3Client, bucketName,
                    keyName, expGrant );
            getObjectByOtherUser();
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == expRunSuccessNum ) {
                CommLib.clearBucket( ownerS3Client, bucketName );
                CommLib.clearUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            ownerS3Client.shutdown();
            userS3Client.shutdown();
        }
    }

    private void getObjectByOtherUser() throws Exception {
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils
                .getMd5OfObject( userS3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
    }

    private void putObjectBeforeTest() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        ownerS3Client.putObject( bucketName, keyName, file );
    }
}
