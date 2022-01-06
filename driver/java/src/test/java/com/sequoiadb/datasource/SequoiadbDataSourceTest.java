package com.sequoiadb.datasource;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

public class SequoiadbDataSourceTest {
    private static SequoiadbDatasource ds;
    private static DatasourceOptions options;
    private static int checkTime = 5 * 1000; // 5s

    @Before
    public void setUp() {
        options = new DatasourceOptions();
        options.setCheckInterval(checkTime);
        options.setMaxIdleCount(20);
        options.setMinIdleCount(10);
        ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", options);
    }

    @After
    public void tearDown() {
        ds.close();
    }

    @Test
    public void createNumTest(){
        try {
            List<Sequoiadb> dbList = new ArrayList<>();
            for (int i = 0; i < options.getMinIdleCount(); i++){
                Sequoiadb db = ds.getConnection();
                dbList.add(db);
            }
            Thread.sleep(checkTime);
            Assert.assertEquals(options.getMinIdleCount(), ds.getIdleConnNum());
            for (int i = 0; i < options.getMinIdleCount(); i++){
                ds.releaseConnection(dbList.get(i));
            }
        }catch (Exception e){
            e.printStackTrace();
        }
    }
}