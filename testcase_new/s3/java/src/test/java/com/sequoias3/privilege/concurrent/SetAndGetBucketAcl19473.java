package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @Description seqDB-19473: 并发配置和获取桶acl
 * @Author wangkexin
 * @Date 2019.09.24
 */
public class SetAndGetBucketAcl19473 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19473";
    private AmazonS3 s3Client = null;
    private String ownerId;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        ownerId = s3Client.getS3AccountOwner().getId();
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        Grant[] defaultGrant = { new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl ) };
        Grant[] expGrant = {
                new Grant( new CanonicalGrantee( ownerId ),
                        Permission.ReadAcp ),
                new Grant( GroupGrantee.AllUsers, Permission.Read ) };

        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadSetBucketAcl( expGrant ) );
        threadExec
                .addWorker( new ThreadGetBucketAcl( defaultGrant, expGrant ) );
        threadExec.run();
        runSuccess = true;

    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private class ThreadSetBucketAcl {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private Grant[] grant;

        public ThreadSetBucketAcl( Grant[] grant ) {
            this.grant = grant;
        }

        @ExecuteOrder(step = 1)
        private void setBucketAcl() {
            try {
                PrivilegeUtils.setBucketAclByBody( s3, bucketName, grant );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadGetBucketAcl {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private Grant[] defaultGrant;
        private Grant[] expGrant;
        private List< Grant > expGrantsList;

        public ThreadGetBucketAcl( Grant[] defaultGrant, Grant[] expGrant ) {
            this.defaultGrant = defaultGrant;
            this.expGrant = expGrant;
        }

        @ExecuteOrder(step = 1)
        private void getBucketAcl() {
            try {
                AccessControlList result = s3.getBucketAcl( bucketName );
                List< Grant > actGrantsList = result.getGrantsAsList();
                if ( actGrantsList.size() == defaultGrant.length ) {
                    expGrantsList = new ArrayList<>(
                            Arrays.asList( defaultGrant ) );
                } else if ( actGrantsList.size() == expGrant.length ) {
                    expGrantsList = new ArrayList<>(
                            Arrays.asList( expGrant ) );
                } else {
                    Assert.fail( "act bucket acl size is wrong : "
                            + actGrantsList.toString() );
                }
                checkGrantList( actGrantsList, expGrantsList );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }

        private void checkGrantList( List< Grant > actGrantsList,
                List< Grant > expGrantsList ) {
            boolean isEqual = false;
            if ( actGrantsList.size() == expGrantsList.size()
                    && actGrantsList.containsAll( expGrantsList )
                    && expGrantsList.containsAll( actGrantsList ) ) {
                isEqual = true;
            }
            if ( !isEqual ) {
                Assert.fail( "bucket acl is wrong! exp grants = "
                        + expGrantsList.toString() + ", act grants = "
                        + actGrantsList.toString() );
            }
        }
    }
}
