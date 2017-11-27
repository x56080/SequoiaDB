package com.sequoiadb.optimize;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:AddHashCodeFromBsonObj13631
 * @author liuxiaoxuan
 * @Date 2017-11-27
 * @version 1.00
 */
public class AddHashCodeFromBsonObj13631 {
    
    @BeforeTest
    public void setUp() {
        System.out.println("Begin TestCase Name:" + this.getClass().getName() + 
                    "at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
    }
    
    @Test
    public void test() {
       
       BSONObject bsonObj1 = new BasicBSONObject();
       bsonObj1.put("_id", 1);
       bsonObj1.put("a", "test1");
       
       BSONObject bsonObj2 = new BasicBSONObject();
       bsonObj2.put("_id", 1);
       bsonObj2.put("a", "test1");
       
       Assert.assertEquals(bsonObj1, bsonObj2);
       Assert.assertEquals(bsonObj1.hashCode(), bsonObj2.hashCode());
       
    }
    
    @AfterTest
    public void tearDown() {
        System.out.println("End TestCase Name:" + this.getClass().getName() + 
                    "at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
    }  
}
