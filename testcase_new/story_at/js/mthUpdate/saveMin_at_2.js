/***************************************************************************************************
 * @Description: $saveMin更新不同类型
 * @ATCaseID: saveMin_at_2
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
 *     $saveMin更新不同类型
 * 测试步骤：
 *    1.$saveMin为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "saveMin_at_2";

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

   var updateDoc1 = { $saveMin: { a: null } };
   dbcl.update(updateDoc1, { no: 1 });
   actual = dbcl.find({ no: 1 });
   commCompareResults(actual, [{ no: 1, a: null }]);

   var updateDoc2 = { $saveMin: { a: 1 } };
   dbcl.update(updateDoc2, { no: 2 });
   actual = dbcl.find({ no: 2 });
   commCompareResults(actual, [{ no: 2, a: 1 }]);

   var updateDoc3 = { $saveMin: { a: "str" } };
   dbcl.update(updateDoc3, { no: 3 });
   actual = dbcl.find({ no: 3 });
   commCompareResults(actual, [{ no: 3, a: "str" }]);

   var updateDoc4 = { $saveMin: { a: { b: 1 } } };
   dbcl.update(updateDoc4, { no: 4 });
   actual = dbcl.find({ no: 4 });
   commCompareResults(actual, [{ no: 4, a: { b: 1 } }]);

   var updateDoc5 = { $saveMin: { a: ["str", 2] } };
   dbcl.update(updateDoc5, { no: 5 });
   actual = dbcl.find({ no: 5 });
   commCompareResults(actual, [{ no: 5, a: 9223372036854 }]);

   var updateDoc18 = { $saveMin: { b: { $field: "a" } } };
   dbcl.update(updateDoc18, { no: 18 });
   actual = dbcl.find({ no: 18 });
   commCompareResults(actual, [{ no: 18, a: 100, b: 100, c: true }]);

   var updateDoc18 = { $saveMin: { a: { $field: "c" } } };
   dbcl.update(updateDoc18, { no: 18 });
   actual = dbcl.find({ no: 18 });
   commCompareResults(actual, [{ no: 18, a: 100, b: 100, c: true }]);
}
