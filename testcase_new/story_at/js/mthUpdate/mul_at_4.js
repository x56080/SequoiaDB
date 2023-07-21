/***************************************************************************************************
 * @Description: $mul混合数值相乘
* @ATCaseID: mul_at_4
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
 *      $mul混合数值相乘
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "mul_at_4";
main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   var actual;
   dbcl.insert({ a: 1, b: { $numberLong: "2" }, c: { $decimal: "3" }, d: 1.2323 });

   dbcl.update({ $mul: { a: 2, b: 2, c: 2 } }, { a: { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 2, b: { $numberLong: "4" }, c: { $decimal: "6" }, d: 1.2323 }]);

   dbcl.update({ $mul: { a: { $numberLong: "2" }, b: 2, c: 2 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 4, b: { $numberLong: 8 }, c: { $decimal: 12 }, d: 1.2323 }]);

   dbcl.update({ $mul: { a: { $numberLong: "2" }, b: { $numberLong: "2" }, c: { $numberLong: "2" } } }, { a: { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 8, b: { $numberLong: "16" }, c: { $decimal: "24" }, d: 1.2323 }]);

   dbcl.update({ $mul: { a: 2147483647, b: { $decimal: "2" }, c: { $decimal: "2" } } }, {});
   actual = dbcl.find({}, { "a": { "$type": 1 } });
   commCompareResults(actual, [{ a: 18, b: { $decimal: "32" }, c: { $decimal: "48" }, d: 1.2323 }]);

   dbcl.update({ $mul: { a: { $numberLong: "9223372036854775807" }, b: { $decimal: "2" }, c: { $decimal: "2" } } }, { a: { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { $decimal: "158456324954741698875069825032" }, b: { $decimal: "64" }, c: { $decimal: "96" }, d: 1.2323 }]);

   dbcl.update({ $mul: { d: 2.2 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { $decimal: "158456324954741698875069825032" }, b: { $decimal: "64" }, c: { $decimal: "96" }, d: 2.71106 }]);

   dbcl.update({ $mul: { d: 2 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { $decimal: "158456324954741698875069825032" }, b: { $decimal: "64" }, c: { $decimal: "96" }, d: 5.42212 }]);

   dbcl.update({ $mul: { d: { $numberLong: "2" } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { $decimal: "158456324954741698875069825032" }, b: { $decimal: "64" }, c: { $decimal: "96" }, d: 10.84424 }]);

   dbcl.update({ $mul: { d: { $decimal: "2" } } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: { $decimal: "158456324954741698875069825032" }, b: { $decimal: "64" }, c: { $decimal: "96" }, d: { $decimal: "21.68848" } }]);

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency(dbcl)

}