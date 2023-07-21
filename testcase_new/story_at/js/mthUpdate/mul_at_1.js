/***************************************************************************************************
 * @Description: $mul更新普通记录
 * @ATCaseID: mul_at_1
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
 *     $mul更新普通记录
 * 测试步骤：
 *    1. $mul作为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "mul_at_1";

main(test);
function test(testPara) {
   var dbcl = testPara.testCL;
   var actual;
   dbcl.insert({ a: 1, b: 2, c: 3, d: 0 });

   dbcl.update({ $mul: { a: 10 } }, { a: { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 10, b: 2, c: 3, d: 0 }]);

   dbcl.update({ $mul: { a: 10 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 100, b: 2, c: 3, d: 0 }]);

   dbcl.update({ $mul: { a: 10 } }, { a: { $exists: 1 } });
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 1000, b: 2, c: 3, d: 0 }]);

   dbcl.update({ $mul: { a: 2, b: 3, c: 4 } }, {});
   actual = dbcl.find();
   commCompareResults(actual, [{ a: 2000, b: 6, c: 12, d: 0 }]);

   db.transBegin();
   dbcl.update({ $mul: { a: 1 } }, {});
   db.transRollback();

   db.transBegin();
   dbcl.update({ $mul: { d: 2 } }, {});
   db.transRollback();

   checkTransactionResidual();

   // 检查主备是否一致
   commCheckLSN(db);
   checkRecordConsistency(dbcl)
}