/***************************************************************************************************
 * @Description: $mul配合$field更新记录
* @ATCaseID: mul_at_3
* @Author: Huang Youquan
* @TestlinkCase: 无
* @Change Activity:
* Date       Who           Description
* ========== ============= =========================================================
* 07/14/2023 Huang Youquan    Init
**************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *     $mul配合$field更新记录
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "mul_at_3";

main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   var actual;
   dbcl.insert({ a: 1, b: 2, c: 3 });

   dbcl.update({ $mul: { a: { $field: "b" } } }, { a: { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 2, b: 2, c: 3 }]);

   dbcl.update({ $set: { b: 2 }, $mul: { a: { $field: "b" } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 4, b: 2, c: 3 }]);

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency(dbcl)
}