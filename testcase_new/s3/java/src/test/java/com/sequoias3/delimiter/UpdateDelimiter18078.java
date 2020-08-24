package com.sequoias3.delimiter;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-18078: create Region by bucket,the object name include old and new delimiter,
 *              than update the old delimiter to the new delimiter.
 * @author wuyan
 * @Date 2019.04.09
 * @version 1.00
 */
public class UpdateDelimiter18078 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18078";
    private String regionName = "region18078";
    private String keyName = "aa%test/maa%/bb%/object18078";
    private String deleteTagkeyName = "aa%delete/aa%/test%/object18078";
    private String delimiter = "test";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private File localPath = null;
    private String filePath = null;
    private SequoiaS3 regionClient = null;

    @SuppressWarnings("deprecation")
    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File(S3TestBase.workDir + File.separator + TestTools.getClassName());
        filePath = localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile(localPath);
        TestTools.LocalFile.createDir(localPath.toString());
        TestTools.LocalFile.createFile(filePath, fileSize);

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket(s3Client, bucketName);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);

        regionClient.createRegion(regionName);
        s3Client.createBucket(bucketName, regionName);
        CommLib.setBucketVersioning(s3Client, bucketName, "Enabled");

        // add a deleteTag key
        s3Client.deleteObject(bucketName, deleteTagkeyName);
        s3Client.putObject(bucketName, keyName, new File(filePath));
    }

    @Test
    public void testUpdateDelimiter() throws Exception {
        DelimiterUtils.putBucketDelimiter(bucketName, delimiter);

        DelimiterUtils.checkCurrentDelimiteInfo(bucketName, delimiter);
        List<String> expCommprefixList = new ArrayList<>();
        expCommprefixList.add("aa%test");
        expCommprefixList.add("aa%delete/aa%/test");
        List<String> expContentList = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter(s3Client, bucketName, delimiter, expCommprefixList, expContentList);

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                CommLib.clearBucket(s3Client, bucketName);
                regionClient.deleteRegion(regionName);
                TestTools.LocalFile.removeFile(localPath);
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }
}
