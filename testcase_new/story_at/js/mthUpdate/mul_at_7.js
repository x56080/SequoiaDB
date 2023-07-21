/***************************************************************************************************
 * @Description: $mul扩展语法验证
* @ATCaseID: mul_at_7
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
 *       $mul扩展语法验证
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "mul_at_7";

main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   var actual;
   dbcl.insert({ a: 1, b: 2, c: 3 });

   dbcl.update({ $mul: { a: { Value: 100, Max: 100 } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 100, b: 2, c: 3 }]);

   dbcl.update({ $mul: { a: { Value: -1, Min: -100 } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: -100, b: 2, c: 3 }]);

   dbcl.update({ $mul: { d: { Value: 100, Default: 200 } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: -100, b: 2, c: 3, d: 20000 }]);

   dbcl.update({ $mul: { e: { Value: -100, Default: null, Max: 50, Min: -50 } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: -100, b: 2, c: 3, d: 20000 }]);

   dbcl.update({ $mul: { f: { Value: 100, Max: 50, Min: -50 } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: -100, b: 2, c: 3, d: 20000, f: 0 }]);

   dbcl.update({ $mul: { g: { Value: -100, Default: 1, Max: 50, Min: -100 } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: -100, b: 2, c: 3, d: 20000, f: 0, g: -100 }]);

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency(dbcl)

}