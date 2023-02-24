/***************************************************************************************************
 * @Description: rest driver get index statistic interface test
 * @ATCaseID: rest_getIndexStat_at_2
 * @Author: Chen Wenjia
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 2023/02/21 Chen Wenjia   Init
 **************************************************************************************************/

/*********************************************testcase***********************************************
 * 环境准备：任意正常集群
 * 测试场景：
 *    获取 index 统计信息，接口缺少一部分参数
 * 测试步骤：
 *    1. 创建一个 cs、cl，并创建 index
 *    2. 通过 get index statistic 接口获取 index 统计信息，缺少 name 参数
 *    3. 通过 get index statistic 接口获取 index 统计信息，缺少 index 参数
 * 期望结果：
 *    步骤2：抛出 SDB_INVALIDARG 异常
 **************************************************************************************************/

main(test);
function test() {
   var csName = "rest_getIndexStat_at_2_cs";
   var clName = "rest_getIndexStat_at_2_cl";
   var indexName = "rest_getIndexStat_at_2_index";
   var fullName = csName + "." + clName;

   commDropCS( db, csName, true, "Drop cs for init" );
   var cl = db.createCS( csName ).createCL( clName );
   cl.createIndex( indexName, { a: 1 } );
   db.analyze();

   tryCatch( [ "cmd=get index statistic", "index=" + indexName ], [ SDB_INVALIDARG ], "Get index statistic should be failed" );
   tryCatch( [ "cmd=get index statistic", "name=" + fullName ], [ SDB_INVALIDARG ], "Get index statistic should be failed" );

   commDropCS( db, csName, true, "Drop cs after test" );
}