/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = TestDBDataCenter.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.dc;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBDataCenter;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testdata.SDBTestHelper;
import com.sequoiadb.test.common.*;
public class TestDBDataCenter {
    private static Sequoiadb sdb;
    private static DBDataCenter dc;
    private static boolean isCluster = false;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        isCluster = Constants.isCluster();
        if(!isCluster){
            return;
        }
        dc  = sdb.getDataCenter();
    }

    @After
    public void tearDown() throws Exception {
        if(!isCluster){
            return;
        }
        dc.activate();
        sdb.disconnect();
    }

    @Test
    public void testGetDetail() {
        if(!isCluster){
            return;
        }
        String name = dc.getName();
        BSONObject detail = dc.getDetail();
        SDBTestHelper.println("name=" + name);
        SDBTestHelper.println(detail.toString());
    }
    
    @Test
    public void testActivate() {
        if(!isCluster){
            return;
        }
        BSONObject detail = dc.getDetail();
        Boolean isActive  = (Boolean)detail.get("Activated");
        assertEquals(isActive, Boolean.TRUE);
        
        dc.deactivate();
        
        detail   = dc.getDetail();
        isActive = (Boolean)detail.get("Activated");
        assertEquals(isActive, Boolean.FALSE);
    }
    
    @Test
    public void testImage() {
        if(!isCluster){
            return;
        }
//        SDBTestHelper.println("before create image");
//        BSONObject detail = dc.getDetail();
//        SDBTestHelper.println(detail.toString());
//        
//        //create image
//        String cataAddrList = "suse-lyb:11853";
//        dc.createImage(cataAddrList);
//        
//        SDBTestHelper.println("after create image");
//        detail = dc.getDetail();
//        SDBTestHelper.println(detail.toString());
//        
//        //attach group {Groups:[["group1", "group1"]]}
//        BSONObject groupInfo = new BasicBSONObject();
//        BSONObject groupPair = new BasicBSONList();
//        groupPair.put("0", "group1");
//        groupPair.put("1", "group1");
//        
//        BSONObject groupItems = new BasicBSONList();
//        groupItems.put("0", groupPair);
//        
//        groupInfo.put("Groups", groupItems);
//        dc.attachGroups(groupInfo);
//        
//        SDBTestHelper.println("after attachGroups");
//        detail = dc.getDetail();
//        SDBTestHelper.println(detail.toString());
//        
//        //disable image
//        dc.disableImage();
//        
//        SDBTestHelper.println("after disable image");
//        detail = dc.getDetail();
//        SDBTestHelper.println(detail.toString());
//        
//        //disable other cluster's readonly mode
//        Sequoiadb db2 = new Sequoiadb("192.168.20.56:11840", "", "");
//        DBDataCenter dc2 = db2.getDataCenter();
//        dc2.enableReadonly();
//
//        //enable image
//        dc.enableImage();
//        
//        SDBTestHelper.println("after enable image");
//        detail = dc.getDetail();
//        SDBTestHelper.println(detail.toString());
//        
//        //remove image
//        dc.removeImage();
//        
//        SDBTestHelper.println("after remove image");
//        detail = dc.getDetail();
//        SDBTestHelper.println(detail.toString());
    }
    
    @Test
    public void testReadonly() {
        if(!isCluster){
            return;
        }
        SDBTestHelper.println("before disable readonly");
        BSONObject detail = dc.getDetail();
        SDBTestHelper.println(detail.toString());
        
        dc.enableReadonly();
        
        SDBTestHelper.println("after enable readonly");
        detail = dc.getDetail();
        Boolean isEnable  = (Boolean)detail.get("Readonly");
        assertEquals(isEnable, Boolean.TRUE);
        SDBTestHelper.println(detail.toString());
        
        dc.disableReadonly();
        
        SDBTestHelper.println("after disable readonly");
        detail = dc.getDetail();
        isEnable  = (Boolean)detail.get("Readonly");
        assertEquals(isEnable, Boolean.FALSE);
        SDBTestHelper.println(detail.toString());
    }
}