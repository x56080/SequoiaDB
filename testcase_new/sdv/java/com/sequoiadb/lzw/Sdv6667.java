package com.sequoiadb.lzw;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6667: 主子表，自动切分数据
 * 1、CL为主子表，且为自动切分表，批量往CL插入不同分区范围的记录 
 * 2、检查返回结果 
 * @Author zengxianquan
 * @Date 2016-12-30
 * @Version 1.00
 */
public class Sdv6667 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String domainName = "domain6667";
    private String csName = "cs6667";
    private String mclName = "mcl6667";
    private String sclName1 = "scl6667_1";
    private String sclName2 = "scl6667_2";
    private String domainRG1 = null;
    private String domainRG2 = null;
    private SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.S");
    
    @BeforeClass
    public void setUp() {
        System.out.println(this.getClass().getName()+" begin at "+sdf.format(new Date()));
        try{
            sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        }catch(BaseException e){            
            Assert.assertTrue(false,"connect failed,"+SdbTestBase.coordUrl+e.getMessage());
        }
        if (Commlib.isStandAlone(sdb)){
            throw new SkipException("is standalone skip testcase");
        }
        if (Commlib.OneGroupMode(sdb)){
            throw new SkipException("less two groups skip testcase");
        }
        try{
            createDomain();
            createCS();
            DBCollection mcl = createMainCL();
            int lowBound = 0;
            int upBound = 2048;
            createAndAttachCL(mcl, sclName1, lowBound, upBound);
            lowBound = 2048;
            upBound = 4096;
            createAndAttachCL(mcl, sclName2, lowBound, upBound);
        }catch(BaseException e){
            Assert.fail(e.getMessage());
        }
    }
    
    @AfterClass(alwaysRun = true)
    public void tearDown(){
        try{
            if(sdb.isCollectionSpaceExist(csName)){
                sdb.dropCollectionSpace(csName);
            }
            if(sdb.isDomainExist(domainName)){
                sdb.dropDomain(domainName);
            }
        }catch(BaseException e){            
            Assert.fail(e.getMessage());
        }finally{
            sdb.disconnect();
            System.out.println(this.getClass().getName()+" end at "+sdf.format(new Date()));
        }
    }
    
    @Test
    public void test() {
        Sequoiadb db = null;
        try{
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            DBCollection mcl = db.getCollectionSpace(csName).getCollection(mclName);
            // insert records covering whole the range
            int dataCount = 4096;
            int strLength =  128*1024;          
            insertData(mcl, dataCount, strLength); 
            // check automatic split on group1
            Sequoiadb dataDB1 = Commlib.getDataDB(db, domainRG1);
            DBCollection scl1 = db.getCollectionSpace(csName).getCollection(sclName1);
            Commlib.waitCreateDict(scl1, domainRG1);
            checkCompression(dataDB1, sclName1);
            checkAutoSplit(dataDB1, sclName1);
            
            DBCollection scl2 = db.getCollectionSpace(csName).getCollection(sclName2);
            Commlib.waitCreateDict(scl2, domainRG1);
            checkCompression(dataDB1, sclName2);
            checkAutoSplit(dataDB1, sclName2);
            
            // check automatic split on group2
            Sequoiadb dataDB2 = Commlib.getDataDB(db, domainRG2);
            Commlib.waitCreateDict(scl1, domainRG2);
            checkCompression(dataDB2, sclName1);
            checkAutoSplit(dataDB2, sclName1);
            
            Commlib.waitCreateDict(scl2, domainRG2);
            checkCompression(dataDB2, sclName2);
            checkAutoSplit(dataDB2, sclName2);
        }catch(BaseException e){
            Assert.fail(e.getMessage());
        }finally{
            db.disconnect();
        }
    }
    
    public String insertData(DBCollection cl, int dataCount, int strLength){
    	String strRec = getRandomString(strLength);
    	for(int i = 0; i < dataCount; i++){
    		cl.insert("{a:"+i+",value:'"+strRec+"'}");
    	}
    	return strRec;
    }
    
    private String getRandomString(int length){
        String base = "abc";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for(int i = 0; i < length; i++){
            int index = random.nextInt(base.length());
            sb.append(base.charAt(index));
        }
        return sb.toString();
    }
    
    private void createDomain(){
        ArrayList<String> dataGroupNames = null;
        dataGroupNames = Commlib.getDataGroups(sdb);
        domainRG1 = dataGroupNames.get(0);
        domainRG2 = dataGroupNames.get(1);
        BSONObject option = new BasicBSONObject();
        BSONObject groups = new BasicBSONList();
        groups.put("0", domainRG1);
        groups.put("1", domainRG2);
        option.put("Groups", groups);
        option.put("AutoSplit", true);
        sdb.createDomain(domainName, option);
    }
    
    private void createCS(){
        BSONObject option = new BasicBSONObject();
        option.put("Domain", domainName);
        sdb.createCollectionSpace(csName, option);
    }
    
    private DBCollection createMainCL(){
        DBCollection mcl = null;
        CollectionSpace cs = sdb.getCollectionSpace(csName);
        BSONObject option = new BasicBSONObject();
        try{
            option.put("ShardingKey", (BSONObject)JSON.parse("{a:1}"));
            option.put("ShardingType", "range");
            option.put("IsMainCL", true);
            mcl = cs.createCollection(mclName, option);
        }catch(BaseException e){
            Assert.fail(e.getMessage());
        }
        return mcl;
    }
    
    private DBCollection createAndAttachCL(DBCollection mcl, String sclName, int lowBound, int upBound){
        DBCollection scl = null;
        CollectionSpace cs = sdb.getCollectionSpace(csName);
        try{
            // create subCL
            BSONObject createOpt = new BasicBSONObject();
            createOpt.put("ShardingKey", (BSONObject)JSON.parse("{a:1}"));
            createOpt.put("ShardingType", "hash");
            createOpt.put("Compressed", true);
            createOpt.put("CompressionType", "lzw");
            scl = cs.createCollection(sclName, createOpt);
            // attach subCL
            BSONObject attachOpt = new BasicBSONObject();
            attachOpt.put("LowBound", (BSONObject)JSON.parse("{a:"+lowBound+"}"));
            attachOpt.put("UpBound", (BSONObject)JSON.parse("{a:"+upBound+"}"));
            mcl.attachCollection(scl.getFullName(), attachOpt);
        }catch(BaseException e){
            Assert.fail(e.getMessage());
        }
        return scl;
    }
    
    private void checkCompression(Sequoiadb dataDB, String clName){
        // get details of snapshot
        BSONObject nameBSON = new BasicBSONObject();
        nameBSON.put("Name", csName + "." + clName);
        DBCursor snapshot = dataDB.getSnapshot(4, nameBSON, null, null);
        BasicBSONList details = (BasicBSONList) snapshot.getNext().get("Details");
        BSONObject detail = (BSONObject) details.get(0);
        
        // judge whether data is compressed
        boolean ratioRight = (double)detail.get("CurrentCompressionRatio") < (double)1;
        boolean attrRight = ((String)detail.get("Attribute")).equals("Compressed");
        boolean typeRight = ((String)detail.get("CompressionType")).equals("lzw");
        if(!(ratioRight && attrRight && typeRight)){
            Assert.fail("data is not compressed");
        }
    }

    private void checkAutoSplit(Sequoiadb dataDB, String clName){
        DBCollection cl = dataDB.getCollectionSpace(csName).getCollection(clName);
        int count = (int)cl.getCount();
        if(count == 0){
            Assert.fail("the auto split does not work");
        }
    }
}