/***************************************************************************************************
 * @Description: rest driver get index statistic interface test
 * @ATCaseID: rest_getIndexStat_at_1
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
 *    获取 index 统计信息
 * 测试步骤：
 *    1. 创建一个 cs、cl，并创建 index
 *    2. 通过 get index statistic 接口获取 index 统计信息
 * 期望结果：
 *    步骤2：可以正常获取到统计信息
 **************************************************************************************************/

main(test);
function test() {
   var csName = "rest_getIndexStat_at_1_cs";
   var clName = "rest_getIndexStat_at_1_cl";
   var indexName = "rest_getIndexStat_at_1_index";
   var fullName = csName + "." + clName;

   commDropCS( db, csName, true, "Drop cs for init" );
   var cl = db.createCS( csName ).createCL( clName );
   cl.createIndex( indexName, { a: 1 } );
   db.analyze();

   tryCatch( [ "cmd=get index statistic", "name=" + fullName, "index=" + indexName ], [ 0 ], "Failed to get index statistic" );
   tryCatch( [ "cmd=get index statistic", "name=" + fullName, "index=" + indexName, "Detail=" + true ], [ 0 ], "Failed to get index statistic" );
   tryCatch( [ "cmd=get index statistic", "name=" + fullName, "index=" + indexName, "Detail=" + false ], [ 0 ], "Failed to get index statistic" );

   commDropCS( db, csName, true, "Drop cs after test" );
}