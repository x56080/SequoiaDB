package com.sequoias3.testcommon.s3utils;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.SkipException;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.S3TestBase;

public class RegionUtils extends S3TestBase {
    private static String pregix = "S3_";
    private static SimpleDateFormat yearFm = new SimpleDateFormat("yyyy");
    private static SimpleDateFormat monthFm = new SimpleDateFormat("MM");

    public static void clearRegion(SequoiaS3 sequoiaS3, String regionName) throws Exception {
        Boolean isRegionExist = sequoiaS3.headRegion(regionName);
        if (isRegionExist) {
            sequoiaS3.deleteRegion(regionName);
        }
    }

    public static void checkRegion(SequoiaS3 regionClient, String regionName, String metaLocation,
            String metaHisLocation, String dataLocation) throws Exception {
        GetRegionResult result = regionClient.getRegion(regionName);
        Region regionInfo = result.getRegion();
        Assert.assertEquals(regionInfo.getMetaLocation(), metaLocation);
        Assert.assertEquals(regionInfo.getMetaHisLocation(), metaHisLocation);
        Assert.assertEquals(regionInfo.getDataLocation(), dataLocation);
    }

    public static AmazonS3Exception httpToAmazon(HttpClientErrorException e) {
        AmazonS3Exception amazonS3Exception = new AmazonS3Exception(e.getMessage());
        amazonS3Exception.setStatusCode(e.getStatusCode().value());
        JSONObject jsonBody = XML.toJSONObject(e.getResponseBodyAsString());
        JSONObject subjsonBody = jsonBody.getJSONObject("Error");
        amazonS3Exception.setErrorCode(subjsonBody.getString("Code"));
        amazonS3Exception.setErrorMessage(subjsonBody.getString("Message"));
        return amazonS3Exception;
    }

    public static void createCSAndCL(String csName, String[] clNames) {
        try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
            if (sdb.isCollectionSpaceExist(csName)) {
                sdb.dropCollectionSpace(csName);
            }
            CollectionSpace cs = sdb.createCollectionSpace(csName);
            for (int i = 0; i < clNames.length; i++) {
                cs.createCollection(clNames[i]);
            }
        }
    }

    public static void dropCS(String[] csNames) {
        try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
            for (int i = 0; i < csNames.length; i++) {
                if (sdb.isCollectionSpaceExist(csNames[i])) {
                    sdb.dropCollectionSpace(csNames[i]);
                }
            }
        }
    }

    public static String getDataCSName(String regionName, DataShardingType shardType, Date currTime) {
        return pregix + regionName + "_DataCS_" + getCsClPostfix(shardType, currTime);
    }

    public static String getMetaCSName(String regionName) {
        return pregix + regionName + "_MetaCS";
    }

    public static String getDataCLName(DataShardingType shardType, Date currTime) {
        return pregix + "ObjectData_" + getCsClPostfix(shardType, currTime);
    }

    public static String getCsClPostfix(DataShardingType shardType, Date currTime) {
        String currY = yearFm.format(currTime);
        String currM = monthFm.format(currTime);
        String postfix = null;
        if (shardType.equals("none")) {
            postfix = "";
        } else if (shardType.equals(DataShardingType.YEAR)) {
            postfix = currY;
        } else if (shardType.equals(DataShardingType.QUARTER)) {
            int quarter = (int) Math.ceil(Double.parseDouble(currM) / 3);
            postfix = "Q" + quarter;
        } else if (shardType.equals(DataShardingType.MONTH)) {
            postfix = currM;
        }
        return postfix;
    }

    public static boolean clInCS(String csName, String clName) {
        Sequoiadb db = null;
        boolean doesCLExist;
        try {
            db = new Sequoiadb(S3TestBase.coordUrl, "", "");
            doesCLExist = db.getCollectionSpace(csName).isCollectionExist(clName);
        } finally {
            if (db != null) {
                db.close();
            }
        }
        return doesCLExist;
    }

    public static boolean doesCSExist(String csName) {
        Sequoiadb db = null;
        boolean flag;
        try {
            db = new Sequoiadb(S3TestBase.coordUrl, "", "");
            flag = db.isCollectionSpaceExist(csName);
        } finally {
            if (db != null) {
                db.close();
            }
        }
        return flag;
    }

    public static void createDomain(String domainName) {
        Sequoiadb sdb = null;
        try {
            sdb = new Sequoiadb(S3TestBase.coordUrl, "", "");
            if (!sdb.isDomainExist(domainName)) {
                List<String> groupList = sdb.getReplicaGroupNames();
                groupList.remove("SYSCatalogGroup");
                groupList.remove("SYSCoord");
                groupList.remove("SYSSpare");
                BSONObject option = new BasicBSONObject();
                BSONObject groups = new BasicBSONList();
                if (groupList.size() < 1) {
                    throw new SkipException("At least one group is required!!! please check " + "env");
                }
                for (int i = 0; i < groupList.size(); i++) {
                    groups.put(String.valueOf(i), groupList.get(i));
                }
                option.put("Groups", groups);
                sdb.createDomain(domainName, option);
            }
        } finally {
            if (null != sdb) {
                sdb.close();
            }
        }
    }

    public static void dropDomain(String domainName) {
        try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
            if (sdb.isDomainExist(domainName)) {
                sdb.dropDomain(domainName);
            }
        }
    }

    public static void dropCS(String csName) {
        try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
            if (sdb.isCollectionSpaceExist(csName)) {
                sdb.dropCollectionSpace(csName);
            }
        }
    }

    public static List<String> listCS(String prefix) {
        List<String> list = new ArrayList<>();
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        try {
            sdb = new Sequoiadb(S3TestBase.coordUrl, "", "");
            cursor = sdb.listCollectionSpaces();
            while (cursor.hasNext()) {
                BasicBSONObject bson = (BasicBSONObject) cursor.getNext();
                if (bson.getString("Name").startsWith(prefix)) {
                    list.add(bson.getString("Name"));
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            if (sdb != null) {
                sdb.close();
            }
        }
        return list;
    }

    public static int getRecordNum(String csName, String clName) {
        Sequoiadb sdb = null;
        int count = 0;
        DBCursor cursor;
        try {
            sdb = new Sequoiadb(S3TestBase.coordUrl, "", "");
            cursor = sdb.getCollectionSpace(csName).getCollection(clName).listLobs();
            while (cursor.hasNext()) {
                cursor.getNext();
                count++;
            }
        } finally {
            if (null != sdb) {
                sdb.close();
            }
        }
        return count;
    }

    public static void checkRegionWithLocation(SequoiaS3 sequoiaS3, String regionName, String metaLocation,
            String metaHisLocation, String dataLocation) throws Exception {
        GetRegionResult result = sequoiaS3.getRegion(regionName);
        Region regionInfo = result.getRegion();
        Assert.assertEquals(regionInfo.getMetaLocation(), metaLocation);
        Assert.assertEquals(regionInfo.getMetaHisLocation(), metaHisLocation);
        Assert.assertEquals(regionInfo.getDataLocation(), dataLocation);
    }

    public static void checkRegionWithShardingType(SequoiaS3 sequoiaS3, String regionName,
            DataShardingType clShardingType, DataShardingType csShardingType) throws Exception {
        GetRegionResult result = sequoiaS3.getRegion(regionName);
        Region regionInfo = result.getRegion();
        Assert.assertEquals(regionInfo.getDataCLShardingType(), clShardingType);
        // get the region infor to take the default value
        Assert.assertEquals(regionInfo.getDataCSShardingType(), csShardingType);
        Assert.assertEquals(regionInfo.getMetaDomain(), null);
        Assert.assertEquals(regionInfo.getDataDomain(), null);
        Assert.assertEquals(regionInfo.getMetaLocation(), null);
        Assert.assertEquals(regionInfo.getMetaHisLocation(), null);
        Assert.assertEquals(regionInfo.getDataLocation(), null);
    }
}
