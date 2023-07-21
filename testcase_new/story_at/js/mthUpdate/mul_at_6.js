/***************************************************************************************************
 * @Description: $mul参数为非数字
* @ATCaseID: mul_at_6
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
 *       $mul参数为非数字
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "mul_at_6";

main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   dbcl.insert({ a: 1, b: 2, c: 3, d: "string" });

   assert.tryThrow(SDB_INVALIDARG, function () {
      dbcl.update({ $mul: { a: "1" } }, { a: { $exists: 1 } })
   });

   assert.tryThrow(SDB_INVALIDARG, function () {
      dbcl.update({ $mul: { a: true } }, { a: { $exists: 1 } })
   }
   );

   assert.tryThrow(SDB_INVALIDARG, function () {
      dbcl.update({ $mul: { a: null } }, { a: { $exists: 1 } })
   }
   );

   // 不报错
   dbcl.update({ $mul: { a: { $field: "d" } } });

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency( dbcl )
}