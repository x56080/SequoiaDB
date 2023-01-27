/***************************************************************************************************
 * @Description: Location 大小不敏感
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: Youquan Huang
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 11/22/2022 Youquan Huang  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含三个节点的复制组
 * 测试场景：
 *    node.setLocation()接口设置Location
 * 测试步骤：
 *    1. 设置全大写字符Location
 *    2. 设置全小写字母Location
 *    3. 设置大小写混合Location
 *    2. 设置中英数字混合Location
 *    3. 设置特殊字符Location

 * 期望结果：
 *    Location大写字符全变成小写
 *
 **************************************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "LOCATION_AT_ONE";
   var location2 = location1.toLowerCase();

   var location3 = "location_at_one";

   var location4 = "location_AT_1";
   var location5 = location4.toLowerCase();

   var location6 = "中文_AT_1";
   var location7 = location6.toLowerCase()

   var location8 = "!@#$%^&*()_+~:,./";

   // 获取一个data节点
   var dataGroupName = commGetDataGroupNames( db )[0];
   var data = db.getRG( dataGroupName ).getSlave();

   data.setLocation( location1 );
   checkNodeLocation( data, location2 );

   data.setLocation( location3 );
   checkNodeLocation( data, location3 );

   data.setLocation( location4 );
   checkNodeLocation( data, location5 );

   data.setLocation( location6 );
   checkNodeLocation( data, location7 );

   data.setLocation( location8 );
   checkNodeLocation( data, location8 );

   // 清理location
   data.setLocation( "" );
}