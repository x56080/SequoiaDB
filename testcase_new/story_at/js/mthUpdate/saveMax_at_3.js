/***************************************************************************************************
 * @Description: $saveMax更新缺失字段
 * @ATCaseID: saveMax_at_3
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 07/27/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *     $saveMax更新缺失字段
 * 测试步骤：
 *    1.$saveMax为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "saveMax_at_3";

main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   var actual;
   dbcl.insert({ a: 1, b: { $minKey: 1 } });

   dbcl.update({ $saveMax: { c: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, b: { $minKey: 1 }, c: 1 }]);

   dbcl.update({ $saveMax: { b: { $field: "d" } } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, c: 1 }]);

   dbcl.update({ $saveMax: { c: { $field: "d" } } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1, c: 1 }]);

   commCheckLSN(db, testPara.srcGroupName);
   checkRecordConsistency(dbcl)

}
