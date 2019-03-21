package com.sequoiadb.subcl;

import java.util.ArrayList;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description	seqDB-101:使用JAVA驱动批量插入时，根据_id进行数据切分(hash) 
 * @author huangxiaoni
 * @date 2019.3.19
 * @review  
 *
 */

public class SubCL101 extends SdbTestBase {
	private Sequoiadb sdb;
	private DBCollection mCL;
	private String srcRg;
	private String trgRg;
	private String domainName = "dm101";
	private String csName = "cs101";
	private String mCLName = "mcl";
	private String sCLName = "scl";
	private int recordsNum = 10000;

	@BeforeClass
	private void setUp() {
		sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		
		if( MetaDataUtils.isStandAlone(sdb) || MetaDataUtils.OneGroupMode(sdb) ) {
			throw new SkipException("The mode is standlone, "
					+ "or only one group, skip the testCase.");
		}
		
		ArrayList<String> groupNames = CommLib.getDataGroupNames(sdb);
		srcRg = groupNames.get(0);
		trgRg = groupNames.get(1);
		
		// create domain
		String[] clRgNames = {srcRg, trgRg};		
		BSONObject dmOpt = new BasicBSONObject();
		dmOpt.put("Groups", clRgNames);
		dmOpt.put("AutoSplit", true);
		sdb.createDomain(domainName, dmOpt);
		
		// create cs
		BSONObject csOpt = new BasicBSONObject();
		csOpt.put("Domain", domainName);
		CollectionSpace cs = sdb.createCollectionSpace(csName, csOpt);
		
		// create mainCL/subCL/attachCL
		mCL = cs.createCollection(mCLName, 
				(BSONObject) JSON.parse("{IsMainCL:true, ShardingKey:{_id:1}}"));
		DBCollection sCL = cs.createCollection(sCLName, 
				(BSONObject) JSON.parse("{ShardingKey:{_id:1}, ShardingType:'hash'}"));
		mCL.attachCollection(sCL.getFullName(), 
				(BSONObject) JSON.parse(
						"{LowBound:{_id:{$minKey:1}}, UpBound:{_id:{$maxKey:1}}}"));
	}

	@Test
	private void test() {
		this.insertData(mCL, recordsNum);
		// check results
		Sequoiadb srcRgDB = null;
		Sequoiadb trgRgDB = null;
		long srcRecCnt = 0;
		long trgRecCnt = 0;
		try {
			srcRgDB = sdb.getReplicaGroup(srcRg).getMaster().connect();
			srcRecCnt = srcRgDB.getCollectionSpace(csName).getCollection(sCLName).getCount();
	
			trgRgDB = sdb.getReplicaGroup(trgRg).getMaster().connect();
			trgRecCnt = trgRgDB.getCollectionSpace(csName).getCollection(sCLName).getCount();
		} finally {
			if (srcRgDB != null) srcRgDB.close();
			if (trgRgDB != null) trgRgDB.close();
		}	
		
		Assert.assertEquals(srcRecCnt + trgRecCnt, recordsNum);
		
		long diffVal = Math.abs(srcRecCnt - trgRecCnt);
		long maxCnt = Math.max(srcRecCnt, trgRecCnt);
		int diffPerc = (int) (((float) diffVal / maxCnt) * 100);
		Assert.assertTrue(diffPerc < 30, 
				"percentage difference, expect 30, but actual " + diffPerc);
	}

	@AfterClass
	private void tearDown() {
		try {
			sdb.dropCollectionSpace(csName);
			sdb.dropDomain(domainName);
		} finally {
			if (sdb != null) {
				sdb.close();
			}
		}
	}

	private void insertData(DBCollection cl, int recordsNum) {
		ArrayList<BSONObject> insertor = new ArrayList<BSONObject>();
		for (int i = 0; i < recordsNum; i++) {
			BSONObject record = new BasicBSONObject();
			record.put("a", i);
			insertor.add(record);
		}
		cl.insert(insertor);
	}
}
