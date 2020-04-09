package com.sequoias3.region;

import org.json.JSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

/**
 * @Description: seqDB-17323 :: 非管理员用户获取区域信息
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class GetRegion17323 extends S3TestBase {
    private String regionName = "region17323";
    private String username = "user17323";
    private String accessKeyID = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( username );
        RegionUtils.clearRegion( regionName );
        JSONObject user = UserUtils.createUser( username, UserCommDefind.normal,
                UserUtils.accessKeyId );
        accessKeyID = user.getJSONObject( UserCommDefind.accessKeys )
                .getString( UserCommDefind.accessKeyID );
    }

    @Test
    private void test() throws Exception {
        // create region
        Region region = new Region();
        region.withDataCSShardingType( "month" )
                .withDataCLShardingType( "month" ).withName( regionName );
        RegionUtils.putRegion( region );

        // get region by normal user
        try {
            RegionUtils.getRegion( regionName, accessKeyID );
            Assert.fail( "exp fail but act success," + "regionName = "
                    + regionName + ",username = " + username );
        } catch ( AmazonS3Exception e ) {
            if ( e.getStatusCode() != 403
                    && !e.getErrorCode().contains( "AccessDenied" ) ) {
                throw e;
            }
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.deleteRegion( regionName );
            CommLib.clearUser( username );
        }
    }
}
