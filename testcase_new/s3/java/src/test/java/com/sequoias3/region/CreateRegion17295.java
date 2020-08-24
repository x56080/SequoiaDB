package com.sequoias3.region;

import java.io.File;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17295: create Region and specify cs and cl. the metaCL and metaHisCL no unique
 *              index
 * @author wuyan
 * @Date 2019.1.21
 * @version 1.00
 */
public class CreateRegion17295 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17295";
    private String key = "key17295";
    private String regionName = "region17295";
    private AmazonS3 s3Client = null;
    private String[] csNames = { "metaCS17295", "dataCS17295" };
    private String[] metaclNames = { "metaCL17295", "metaHistroyCL17295" };
    private String[] dataclNames = { "dataCL17295" };
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(csNames[0], metaclNames);
        RegionUtils.createCSAndCL(csNames[1], dataclNames);

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket(s3Client, bucketName);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    public void testRegion() throws Exception {

        String metaLocation = csNames[0] + "." + metaclNames[0];
        String metaHisLocation = csNames[0] + "." + metaclNames[1];
        String dataLocation = csNames[1] + "." + dataclNames[0];
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withMetaLocation(metaLocation).withDataLocation(dataLocation).withMetaHisLocation(metaHisLocation);
        regionClient.createRegion(request);

        // get region and check region info
        RegionUtils.checkRegion(regionClient, regionName, metaLocation, metaHisLocation, dataLocation);

        // check auto create Index
        String metaIndexKey = "{ \"BucketId\" : 1 , \"Key\" : 1 }";
        String metaHisIndexKey = "{ \"BucketId\" : 1 , \"Key\" : 1 , \"VersionId\" : 1 }";
        checkIndex(csNames[0], metaclNames[0], "BucketId_Key", metaIndexKey);
        checkIndex(csNames[0], metaclNames[1], "BucketId_Key_VersionId", metaHisIndexKey);

        // create object on region
        createObjectAndCheckResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                CommLib.clearBucket(s3Client, bucketName);
                regionClient.deleteRegion(regionName);
                RegionUtils.dropCS(csNames);
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult() throws Exception {
        s3Client.createBucket(bucketName, regionName);
        CommLib.setBucketVersioning(s3Client, bucketName, "Enabled");
        for (int i = 0; i < 10; i++) {
            String context = "testcreatekeyonregion17295" + "_test" + i;
            s3Client.putObject(bucketName, key, context);
            String version = i + "";
            File localPath = new File(S3TestBase.workDir + File.separator + TestTools.getClassName());
            String downfileMd5 = ObjectUtils.getMd5OfObject(s3Client, localPath, bucketName, key, version);
            Assert.assertEquals(downfileMd5, TestTools.getMD5(context.getBytes()));
            TestTools.LocalFile.removeFile(localPath);
        }
    }

    private void checkIndex(String csName, String clName, String indexName, String indexKey) {
        try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
            DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
            Assert.assertTrue(cl.isIndexExist(indexName));
            DBCursor cur = cl.getIndexes();

            int autoCreateIndexNum = 0;
            while (cur.hasNext()) {
                BSONObject obj = cur.getNext();
                BSONObject indexInfo = (BSONObject) obj.get("IndexDef");
                String name = (String) indexInfo.get("name");
                if (name.equals(indexName)) {
                    String actIndexKey = indexInfo.get("key").toString();
                    boolean isUnique = (boolean) indexInfo.get("unique");
                    boolean isEnforced = (boolean) indexInfo.get("enforced");
                    Assert.assertEquals(actIndexKey, indexKey);
                    Assert.assertTrue(isUnique);
                    Assert.assertTrue(isEnforced);
                    autoCreateIndexNum++;
                }
            }
            // auto create one unique index.
            Assert.assertEquals(autoCreateIndexNum, 1);
        }
    }

}
