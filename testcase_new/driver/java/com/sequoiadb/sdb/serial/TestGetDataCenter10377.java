package com.sequoiadb.sdb.serial;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBDataCenter;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestGetDataCenter10377 extends SdbTestBase {

    @Test
    public void testDataCenter() {
        String coordAddr = SdbTestBase.coordUrl;
        try {
            Sequoiadb sdb = new Sequoiadb( coordAddr, "", "" );
            DBDataCenter dc = sdb.getDataCenter();
            // activate
            dc.deactivate();
            BSONObject detail = dc.getDetail();
            boolean isActive = ( Boolean ) detail.get( "Activated" );
            Assert.assertEquals( isActive, false );
            dc.activate();
            try {
                Thread.sleep( 1000 * 50 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
            detail = dc.getDetail();
            isActive = ( Boolean ) detail.get( "Activated" );
            Assert.assertEquals( isActive, true );

            dc.enableReadonly();
            detail = dc.getDetail();
            boolean isEnable = ( Boolean ) detail.get( "Readonly" );
            Assert.assertEquals( isEnable, true );
            dc.disableReadonly();
            try {
                Thread.sleep( 1000 * 50 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
            detail = dc.getDetail();
            isEnable = ( Boolean ) detail.get( "Readonly" );
            Assert.assertEquals( isEnable, false );
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestGetDataCenter10377 testDataCenter error, error description:"
                            + e.getMessage() );
        }
    }
}
