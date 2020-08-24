package com.sequoias3.region;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.ListRegionsResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17317:创建多个区域，获取区域列表 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class GetRegionList17317 extends S3TestBase {
    private String regionName = "Beijing17317";
    private String dataDomain = "dataDomain17317";
    private String metaDomain = "metaDomain17317";
    private String metaCSName = "metaCS17317";
    private String dataCSName = "dataCS17317";
    private String[] metaClNames = { "metaCL17317", "metaHistoryCL17317" };
    private String[] dataClName = { "dataCL17317" };
    private List<String> regionNames = new ArrayList<>();
    private int regionNum = 50;
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.dropDomain(metaDomain);
        RegionUtils.dropDomain(dataDomain);

        RegionUtils.createCSAndCL(metaCSName, metaClNames);
        RegionUtils.createCSAndCL(dataCSName, dataClName);

        RegionUtils.createDomain(dataDomain);
        RegionUtils.createDomain(metaDomain);
        regionClient = CommLib.regionClient();
        for (int i = 0; i < regionNum; i++) {
            String currRegionName = regionName + "-" + i;
            regionNames.add(currRegionName.toLowerCase());
            RegionUtils.clearRegion(regionClient, regionName);
        }
    }

    @Test
    public void testCreateRegion() throws Exception {
        // create regions
        for (int i = 0; i < regionNum / 2; i++) {
            CreateRegionRequest request = new CreateRegionRequest(regionNames.get(i));
            request.withMetaLocation(metaCSName + "." + metaClNames[0])
                    .withMetaHisLocation(metaCSName + "." + metaClNames[1])
                    .withDataLocation(dataCSName + "." + dataClName[0]);
            regionClient.createRegion(request);
        }

        for (int i = regionNum / 2; i < regionNum; i++) {
            CreateRegionRequest request = new CreateRegionRequest(regionNames.get(i));
            request.withDataCSShardingType(DataShardingType.QUARTER).withDataCLShardingType(DataShardingType.MONTH)
                    .withDataDomain(dataDomain).withMetaDomain(metaDomain);
            regionClient.createRegion(request);
        }

        ListRegionsResult result = regionClient.listRegions();
        List<String> actRegions = result.getRegions();
        Set<String> unRepeatRegionNames = new HashSet<>();
        for (String regionName : actRegions) {
            boolean isRepeat = unRepeatRegionNames.add(regionName);
            Assert.assertTrue(isRepeat, "the region name " + regionName + " is repeated!");
        }
        Assert.assertTrue(actRegions.containsAll(regionNames),
                " expect regions is : " + regionNames.toString() + ", act regions is : " + actRegions.toString());

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if (runSuccess) {
            try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
                sdb.dropCollectionSpace(metaCSName);
                sdb.dropCollectionSpace(dataCSName);
                deleteRegions(regionNames);
                sdb.dropDomain(dataDomain);
                sdb.dropDomain(metaDomain);
            } finally {
                regionClient.shutdown();
            }
        }
    }

    private void deleteRegions(List<String> regions) throws Exception {
        for (int i = 0; i < regions.size(); i++) {
            if (regionClient.headRegion(regions.get(i))) {
                regionClient.deleteRegion(regions.get(i));
            }
        }
    }
}
