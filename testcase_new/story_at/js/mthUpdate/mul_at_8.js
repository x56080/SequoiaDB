/***************************************************************************************************
 * @Description: 集合开启StrictDataMode
* @ATCaseID: mul_at_8
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
 *       集合开启StrictDataMode
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "mul_at_8";

main(test);

function test(testPara) {

   var dbcl = testPara.testCL;
   var actual;
   dbcl.alter({ StrictDataMode: true });
   dbcl.insert({ a: 2, b: { $numberLong: "2" }, c: { $decimal: "100" }, d: { $decimal: "1.213", $precision: [4, 3] } });

   assert.tryThrow(SDB_VALUE_OVERFLOW, function () {
      dbcl.update({ $mul: { a: 2147483647, b: { $decimal: "2" }, c: { $decimal: "2" } } }, {});
   });

   assert.tryThrow(SDB_VALUE_OVERFLOW, function () {
      dbcl.update({ $mul: { a: 1, b: { $numberLong: "9223372036854775807" }, c: { $decimal: "2" } } }, {});
   });

   var arr = new Array(131072);
   var dights = arr.join("1");
   assert.tryThrow(SDB_INVALIDARG, function () {
      dbcl.update({ $mul: { a: 1, b: { $numberLong: "2" }, c: { $decimal: dights } } }, {});
   });

   dbcl.update({ $mul: { d: { $decimal: "1.213", $precision: [4, 3] } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 2, b: { $numberLong: "2" }, c: { $decimal: "100" }, d: { $decimal: "1.471369" } }]);

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency(dbcl)
}

