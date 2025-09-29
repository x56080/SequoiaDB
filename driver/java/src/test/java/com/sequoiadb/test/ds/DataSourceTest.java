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

   Source File Name = DataSourceTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.ds;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DataSource;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

public class DataSourceTest {

    private static Sequoiadb sdb;
    private static DataSource ds;
    private static String errorName = "error_datasource_name";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        ds = sdb.createDataSource(Constants.DATASOURCE_NAME, Constants.DATASOURCE_ADDRESS, "", "",
                Constants.DATASOURCE_TYPE, null);
    }

    @AfterClass
    public static void DropAfterClass() throws Exception {
        sdb.dropDataSource(ds.getName());
        sdb.close();
    }

    @Test
    public void dataSourceExistTest() {
        Assert.assertFalse(sdb.isDataSourceExist(errorName));
    }

    @Test
    public void createDataSourceTest() {
        String dsName = "dataSourceTestJava";

        // case 1: type is empty string and use urls
        sdb.createDataSource(dsName, Constants.DATASOURCE_URLS, "", "",
                "", null);
        Assert.assertTrue(sdb.isDataSourceExist(dsName));
        sdb.dropDataSource(dsName);

        // case 2: type is error string
        try {
            sdb.createDataSource(dsName, Constants.DATASOURCE_ADDRESS, "", "",
                    "errorTypeTest", null);
        }catch (BaseException e){
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 3: empty option
        BSONObject option1 = new BasicBSONObject();
        sdb.createDataSource(dsName, Constants.DATASOURCE_ADDRESS, "", "",
                Constants.DATASOURCE_TYPE, option1);
        Assert.assertTrue(sdb.isDataSourceExist(dsName));
        sdb.dropDataSource(dsName);

        // case 4: invalid option
        try {
            BSONObject option2 = new BasicBSONObject("a",1);
            sdb.createDataSource(dsName, Constants.DATASOURCE_ADDRESS, "", "",
                    Constants.DATASOURCE_TYPE, option2);
        }catch (BaseException e){
            Assert.assertEquals(SDBError.SDB_OPTION_NOT_SUPPORT.getErrorCode(), e.getErrorCode());
        }

        // case 5: valid option
        BSONObject option3 = new BasicBSONObject();
        option3.put("AccessMode", "READ");
        option3.put("ErrorFilterMask", "READ");
        option3.put("ErrorControlLevel", "Low");
        sdb.createDataSource(dsName, Constants.DATASOURCE_ADDRESS, "", "",
                Constants.DATASOURCE_TYPE, option3);
        // check data
        DBCursor cursor = sdb.listDataSources(new BasicBSONObject("Name", dsName),
                null, null, null);
        BSONObject obj = cursor.getNext();
        Assert.assertEquals("READ", obj.get("AccessModeDesc"));
        Assert.assertEquals("READ", obj.get("ErrorFilterMaskDesc"));
        Assert.assertEquals("Low", obj.get("ErrorControlLevel"));
        sdb.dropDataSource(dsName);
    }

    @Test
    public void dropErrorDataSourceTest() {
        try {
            sdb.dropDataSource(errorName);
        }catch (BaseException e){
            Assert.assertEquals(SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void getDataSourceTest() {
        sdb.getDataSource(ds.getName());

        try {
            sdb.getDataSource(errorName);
        }catch (BaseException e){
            Assert.assertEquals( SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void alterDataSourceTest() {
        // case 1: empty option
        try {
            ds.alterDataSource(new BasicBSONObject());
        }catch (BaseException e){
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 2: invalid option
        try {
            ds.alterDataSource(new BasicBSONObject("a","1"));
        }catch (BaseException e){
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 3: valid option
        String dsNewName = "dataSourceNewName";
        BSONObject option4 = new BasicBSONObject();
        option4.put("Name",dsNewName);
        option4.put("AccessMode", "READ");
        option4.put("ErrorFilterMask", "READ");
        option4.put("ErrorControlLevel", "Low");
        option4.put("Address", Constants.DATASOURCE_URLS);
        ds.alterDataSource(option4);

        Assert.assertEquals(dsNewName, ds.getName());

        BSONObject mather = new BasicBSONObject("Name", ds.getName());
        DBCursor cursor = sdb.listDataSources(mather, null, null, null);
        if (!cursor.hasNext()){
            throw new BaseException(SDBError.SDB_CAT_DATASOURCE_NOTEXIST, "no data");
        }
        BSONObject obj = cursor.getNext();
        Assert.assertEquals("READ", obj.get("AccessModeDesc"));
        Assert.assertEquals("READ", obj.get("ErrorFilterMaskDesc"));
        Assert.assertEquals("Low", obj.get("ErrorControlLevel"));
        Assert.assertEquals(Constants.DATASOURCE_URLS, obj.get("Address"));
    }
}