/***************************************************************************************************
 * @Description: $mul缺少字段
* @ATCaseID: mul_at_5
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
 *       $mul缺少字段
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "mul_at_5";

main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   var actual;

   dbcl.insert({ a: 1, b: 2, c: 3, d: [1, 2, 3], e: { f: 1, g: 2 }, i: "str" });

   dbcl.update({ $mul: { x: 1 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, b: 2, c: 3, d: [1, 2, 3], e: { f: 1, g: 2 }, i: "str", x: 0 }]);

   dbcl.update({ $mul: { "b.1": 1 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, b: 2, c: 3, d: [1, 2, 3], e: { f: 1, g: 2 }, i: "str", x: 0 }]);

   dbcl.update({ $mul: { "d.3": 1 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, b: 2, c: 3, d: [1, 2, 3, 0], e: { f: 1, g: 2 }, i: "str", x: 0 }]);

   dbcl.update({ $mul: { "e.h": 1 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, b: 2, c: 3, d: [1, 2, 3, 0], e: { f: 1, g: 2, h: 0 }, i: "str", x: 0 }]);

   dbcl.update({ $mul: { a: { $field: "z" } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, b: 2, c: 3, d: [1, 2, 3, 0], e: { f: 1, g: 2, h: 0 }, i: "str", x: 0 }]);

   dbcl.update({ $mul: { j: { $field: "i" } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, b: 2, c: 3, d: [1, 2, 3, 0], e: { f: 1, g: 2, h: 0 }, i: "str", x: 0, j: 0 }]);

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency(dbcl)
}