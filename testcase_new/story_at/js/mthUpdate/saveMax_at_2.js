/***************************************************************************************************
 * @Description: $saveMax更新不同类型
 * @ATCaseID: saveMax_at_2
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
 *     $saveMax更新不同类型
 * 测试步骤：
 *    1.$saveMax为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "saveMax_at_2";

main(test);
function test(testPara) {

   var dbcl = testPara.testCL;
   var actual;
   // insert different types of data
   var docs = [
      { no: 1, a: 1 },
      { no: 2, a: "str" },
      { no: 3, a: { b: 1 } },
      { no: 4, a: ["str", 2] },
      { no: 5, a: 9223372036854 },
      { no: 6, a: { $date: "2021-01-01" } },
      { no: 7, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
      { no: 8, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "255" } },
      { no: 9, a: { $decimal: "100.01" } },
      { no: 10, a: 100.01 },
      { no: 11, a: { b: { c: { d: 1 } } } },
      { no: 12, a: [[1, 2], 3] },
      { no: 13, a: { $regex: ".*" } },
      { no: 14, a: { $minKey: 1 } },
      { no: 15, a: { $maxKey: 1 } },
      { no: 16, a: { $undefined: 1 } },
      { no: 17, a: null },
      { no: 18, a: 100, b: "str", c: true },
   ];

   dbcl.insert(docs);

   var updateDoc1 = { $saveMax: { a: "str" } };
   dbcl.update(updateDoc1, { no: 1 });
   actual = dbcl.find({ no: 1 });
   commCompareResults(actual, [{ no: 1, a: "str" }]);

   var updateDoc2 = { $saveMax: { a: { b: 1 } } };
   dbcl.update(updateDoc2, { no: 2 });
   actual = dbcl.find({ no: 2 });
   commCompareResults(actual, [{ no: 2, a: { b: 1 } }]);

   var updateDoc3 = { $saveMax: { a: true } };
   dbcl.update(updateDoc3, { no: 3 });
   actual = dbcl.find({ no: 3 });
   commCompareResults(actual, [{ no: 3, a: true }]);

   var updateDoc4 = { $saveMax: { a: { $maxKey: 1 } } };
   dbcl.update(updateDoc4, { no: 4 });
   actual = dbcl.find({ no: 4 });
   commCompareResults(actual, [{ no: 4, a: { $maxKey: 1 } }]);

   var updateDoc5 = { $saveMax: { a: { $minKey: 1 } } };
   dbcl.update(updateDoc5, { no: 5 });
   actual = dbcl.find({ no: 5 });
   commCompareResults(actual, [{ no: 5, a: 9223372036854 }]);

   var updateDoc6 = { $saveMax: { a: 100.01 } };
   dbcl.update(updateDoc6, { no: 6 });
   actual = dbcl.find({ no: 6 });
   commCompareResults(actual, [{ no: 6, a: { $date: "2021-01-01" } }]);

   var updateDoc9 = { $saveMax: { a: 100.01 } };
   dbcl.update(updateDoc9, { no: 9 });
   actual = dbcl.find({ no: 9 });
   commCompareResults(actual, [{ no: 9, a: { $decimal: "100.01" } }]);

   var updateDoc18 = { $saveMax: { a: { $field: "c" } } };
   dbcl.update(updateDoc18, { no: 18 });
   actual = dbcl.find({ no: 18 });
   commCompareResults(actual, [{ no: 18, a: true, b: "str", c: true }]);

   var updateDoc18 = { $saveMax: { a: { $field: "b" } } };
   dbcl.update(updateDoc18, { no: 18 });
   actual = dbcl.find({ no: 18 });
   commCompareResults(actual, [{ no: 18, a: true, b: "str", c: true }]);

   commCheckLSN(db, testPara.srcGroupName );
   checkRecordConsistency(dbcl);
}
