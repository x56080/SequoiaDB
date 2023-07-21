/***************************************************************************************************
 * @Description: $mul更新普通记录
 * @ATCaseID: mul_at_2
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
 *     $mul更新镶嵌记录
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "mul_at_2";

main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   var actual;
   dbcl.insert({ a: { b: 1, c: 2, d: 3 }, b: [1, 2, 3] });
   dbcl.update({ $mul: { "a.b": 10 } }, { "a.b": { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { b: 10, c: 2, d: 3 }, b: [1, 2, 3] }]);

   dbcl.update({ $mul: { "a.b": 10 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { b: 100, c: 2, d: 3 }, b: [1, 2, 3] }]);

   dbcl.update({ $mul: { "b.1": 10 } }, { "b.1": { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { b: 100, c: 2, d: 3 }, b: [1, 20, 3] }]);

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency(dbcl)
}