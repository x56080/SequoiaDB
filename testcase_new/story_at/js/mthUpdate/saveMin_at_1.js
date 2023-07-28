/***************************************************************************************************
 * @Description: $saveMin更新同种类型
 * @ATCaseID: saveMin_at_1
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
 *     $saveMin更新同种类型
 * 测试步骤：
 *    1.$saveMin为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "saveMin_at_1";

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
      { no: 8, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      { no: 9, a: { $decimal: "100.01" } },
      { no: 10, a: 100.01 },
      { no: 11, a: { b: { c: { d: 1 } } } },
      { no: 12, a: [[1, 2], 3] },
      { no: 13, a: { $regex: ".*" } },
      { no: 14, a: { $minKey: 1 } },
      { no: 15, a: { $maxKey: 1 } },
      { no: 16, a: { $undefined: 1 } },
      { no: 17, a: null },
      { no: 18, a: 100, b: 200, c: 300 },
      { no: 19, a: "a", b: "b", c: "c" },
      { no: 20, a: false, b: true, c: false },
   ];
   dbcl.insert(docs);

   // update data
   var updateDoc1 = { $saveMin: { a: 0 } };
   dbcl.update(updateDoc1, { no: 1 });
   // check result
   actual = dbcl.find({ no: 1 });
   commCompareResults(actual, [{ no: 1, a: 0 }]);

   var updateDoc2 = { $saveMin: { a: "atr" } };
   dbcl.update(updateDoc2, { no: 2 });
   actual = dbcl.find({ no: 2 });
   commCompareResults(actual, [{ no: 2, a: "atr" }]);

   var updateDoc3 = { $saveMin: { a: { b: 0 } } };
   dbcl.update(updateDoc3, { no: 3 });
   actual = dbcl.find({ no: 3 });
   commCompareResults(actual, [{ no: 3, a: { b: 0 } }]);

   var updateDoc4 = { $saveMin: { a: ["atr", 0] } };
   dbcl.update(updateDoc4, { no: 4 });
   actual = dbcl.find({ no: 4 });
   commCompareResults(actual, [{ no: 4, a: ["atr", 0] }]);

   var updateDoc5 = { $saveMin: { a: { $decimal: "9223372036853.02" } } };
   dbcl.update(updateDoc5, { no: 5 });
   actual = dbcl.find({ no: 5 });
   commCompareResults(actual, [{ no: 5, a: { $decimal: "9223372036853.02" } }]);

   var updateDoc6 = { $saveMin: { a: { $date: "2020-01-01" } } };
   dbcl.update(updateDoc6, { no: 6 });
   actual = dbcl.find({ no: 6 });
   commCompareResults(actual, [{ no: 6, a: { $date: "2020-01-01" } }]);

   var updateDoc7 = { $saveMin: { a: { $timestamp: "2037-12-31-23.59.59.999997" } } };
   dbcl.update(updateDoc7, { no: 7 });
   actual = dbcl.find({ no: 7 });
   commCompareResults(actual, [{ no: 7, a: { $timestamp: "2037-12-31-23.59.59.999997" } }]);

   var updateDoc18 = { $saveMin: { b: { $field: "a" } } };
   dbcl.update(updateDoc18, { no: 18 });
   actual = dbcl.find({ no: 18 });
   commCompareResults(actual, [{ no: 18, a: 100, b: 100, c: 300 }]);

   var updateDoc19 = { $saveMin: { b: { $field: "a" } } };
   dbcl.update(updateDoc19, { no: 19 });
   actual = dbcl.find({ no: 19 });
   commCompareResults(actual, [{ no: 19, a: "a", b: "a", c: "c" }]);

   var updateDoc20 = { $saveMin: { a: { $field: "b" } } };
   dbcl.update(updateDoc20, { no: 20 });
   actual = dbcl.find({ no: 20 });
   commCompareResults(actual, [{ no: 20, a: false, b: true, c: false }]);
}