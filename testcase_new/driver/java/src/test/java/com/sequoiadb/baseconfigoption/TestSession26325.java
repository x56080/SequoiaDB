package com.sequoiadb.baseconfigoption;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @descreption seqDB-26325:Prefered更正为Preferred验证
 * @author ZhangYanan
 * @date 2022/04/05
 * @updateUser ZhangYanan
 * @updateDate 2022/04/05
 * @updateRemark
 * @version 1.0
 */
public class TestSession26325 extends SdbTestBase {
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setSdb() {
        // 建立 SequoiaDB 数据库连接
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone" );
        }
    }

    @Test
    public void test() {
        String preferred = "PreferredInstance";
        sdb.setSessionAttr( new BasicBSONObject( preferred, "-S" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( preferred ), "-S" );

        String preferredMode = "PreferredInstanceMode";
        sdb.setSessionAttr( new BasicBSONObject( preferredMode, "ordered" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( preferredMode ),
                "ordered" );
    }

    @AfterClass
    public void closeSdb() {
        if ( sdb != null ) {
            sdb.close();
        }
    }
}