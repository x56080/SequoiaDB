package com.sequoiadb.decimal;

import java.math.BigDecimal;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class Decimal10238 extends SdbTestBase{
	private Sequoiadb sdb ;
	private SimpleDateFormat df = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS");
	
	private CollectionSpace cs = null;
	private String clName = "cl10238";
	private DBCollection cl = null;
	private String coordAddr;
	private String commCSName;
	
	@BeforeClass
	public void setUp(){
		this.coordAddr = SdbTestBase.coordUrl;
		this.commCSName = SdbTestBase.csName;
		try{
			sdb = new Sequoiadb(coordAddr,"","");
			if(!sdb.isCollectionSpaceExist(commCSName)){
				sdb.createCollectionSpace(commCSName);
			}
			cs = sdb.getCollectionSpace(commCSName);
			if(cs.isCollectionExist(clName)){
				cs.dropCollection(clName);
			}
			cl = cs.createCollection(clName);
		}catch(BaseException e){
			Assert.fail("prepare env failed" + e.getMessage());
		}
	}
		
	@AfterClass
	public void tearDown(){
		try{
			if(cs.isCollectionExist(clName)){
				cs.dropCollection(clName);
			}
			sdb.disconnect();
		}catch(BaseException e){
			Assert.fail("clear env failed, errMsg:" + e.getMessage());
		}
	}
	
	@DataProvider(name = "DataProviderWithPrecision")
	public Object[][] createDataWithPrecision(){
		return new Object[][]{
			//seqDB-9581
			{"max","MAX",2,0},
//			{"min","MIN",2,0},
//			{"nan","NaN",2,0},
//			{"MAX","MAX",1000,999},
//			{"MIN","MIN",1000,999},
//			{"NAN","NaN",1000,999},
		};
	}
	
	@Test(dataProvider = "DataProviderWithPrecision")
	public void test(String value, String expectValue, int precision, int scale){
		//insert data and check
		insertAndCheckDecimal(value, expectValue, precision, scale);
		
	}
	
	public void insertAndCheckDecimal(String value, String expectValue,
			int precision, int scale ){
		BSONObject obj = new BasicBSONObject();
		try{
//			BigDecimal expectBigDecimal = new BigDecimal(expectValue);
			BSONDecimal data = new BSONDecimal(value, precision, scale);
			BSONDecimal data1 = new BSONDecimal("1.7e+1000");
			obj.put("a", data);
			cl.insert(obj);
			BSONDecimal actualData = (BSONDecimal) cl.queryOne().get("a");
//			BigDecimal actualBigDecimal = actualData.toBigDecimal();
			String actualValue = data.getValue();
			int actualPrecision = data.getPrecision();
			int actualScale = data.getScale();
			System.out.println("data:"+data);
			System.out.println("actualData:"+actualData);
			System.out.println("actualValue:"+actualValue);
			System.out.println("expectValue:"+expectValue);
			System.out.println("actualPrecision:"+actualPrecision);
			System.out.println("actualScale:"+actualScale);
			Assert.assertEquals(actualData, data);
			Assert.assertEquals(actualValue, expectValue);
			if(actualData.compareTo(data1) != 1){
				Assert.fail("compareTo failed, errMsg:");
			}
//			Assert.assertEquals(actualPrecision, -1);
//			Assert.assertEquals(actualScale, -1);
//			if(actualBigDecimal.compareTo(expectBigDecimal) != 0){
//				Assert.fail("compare bigDecimal data failed,expect data:" + expectBigDecimal + 
//						",actual data: " + actualBigDecimal);
//			}
//			cl.truncate();
		}catch(IllegalArgumentException e ){
			Assert.fail("generate data:" + obj + " failed, errMsg:" + e.getMessage());
		}catch(BaseException e){
			Assert.fail("insert data:" + obj + " failed, errMsg:" + e.getMessage());
		}
	}
}
