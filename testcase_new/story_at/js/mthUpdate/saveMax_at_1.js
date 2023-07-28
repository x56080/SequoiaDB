/***************************************************************************************************
 * @Description: $saveMax更新同种类型
 * @ATCaseID: saveMax_at_1
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
 *     $saveMax更新同种类型
 * 测试步骤：
 *    1.$saveMax为更新符，发起更新操作
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "saveMax_at_1";

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
      { no: 18, a: 100, b: 200, c: 100 },
      { no: 19, a: "a", b: "b", c: "c" },
      { no: 20, a: false, b: true, c: false },
   ];
   dbcl.insert(docs);

   // update data
   var updateDoc1 = { $saveMax: { a: 2 } };
   dbcl.update(updateDoc1, { no: 1 });
   // check result
   actual = dbcl.find({ no: 1 });
   commCompareResults(actual, [{ no: 1, a: 2 }]);

   var updateDoc2 = { $saveMax: { a: "ttr" } };
   dbcl.update(updateDoc2, { no: 2 });
   actual = dbcl.find({ no: 2 });
   commCompareResults(actual, [{ no: 2, a: "ttr" }]);

   var updateDoc3 = { $saveMax: { a: { b: 2 } } };
   dbcl.update(updateDoc3, { no: 3 });
   actual = dbcl.find({ no: 3 });
   commCompareResults(actual, [{ no: 3, a: { b: 2 } }]);

   var updateDoc4 = { $saveMax: { a: ["str", 3] } };
   dbcl.update(updateDoc4, { no: 4 });
   actual = dbcl.find({ no: 4 });
   commCompareResults(actual, [{ no: 4, a: ["str", 3] }]);

   var updateDoc5 = { $saveMax: { a: 9223372036854.12 } };
   dbcl.update(updateDoc5, { no: 5 });
   actual = dbcl.find({ no: 5 });
   commCompareResults(actual, [{ no: 5, a: 9223372036854.12 }]);

   var updateDoc6 = { $saveMax: { a: { $date: "2021-01-02" } } };
   dbcl.update(updateDoc6, { no: 6 });
   actual = dbcl.find({ no: 6 });
   commCompareResults(actual, [{ no: 6, a: { $date: "2021-01-02" } }]);

   var updateDoc7 = { $saveMax: { a: { $date: "2038-12-31" } } };
   dbcl.update(updateDoc7, { no: 7 });
   actual = dbcl.find({ no: 7 });
   commCompareResults(actual, [{ no: 7, a: { $date: "2038-12-31" } }]);

   var updateDoc8 = { $saveMax: { a: { $binary: "bGVsbG8gd29ybGQ=", $type: "1" } } };
   dbcl.update(updateDoc8, { no: 8 });
   actual = dbcl.find({ no: 8 });
   commCompareResults(actual, [{ no: 8, a: { $binary: "bGVsbG8gd29ybGQ=", $type: "1" } }]);

   var updateDoc9 = { $saveMax: { a: { $decimal: "100.02" } } };
   dbcl.update(updateDoc9, { no: 9 });
   actual = dbcl.find({ no: 9 });
   commCompareResults(actual, [{ no: 9, a: { $decimal: "100.02" } }]);

   var updateDoc10 = { $saveMax: { a: { $decimal: "100.02" } } };
   dbcl.update(updateDoc10, { no: 10 });
   actual = dbcl.find({ no: 10 });
   commCompareResults(actual, [{ no: 10, a: { $decimal: "100.02" } }]);

   var updateDoc11 = { $saveMax: { a: { b: { c: { d: 2 } } } } };
   dbcl.update(updateDoc11, { no: 11 });
   actual = dbcl.find({ no: 11 });
   commCompareResults(actual, [{ no: 11, a: { b: { c: { d: 2 } } } }]);

   var updateDoc12 = { $saveMax: { a: [[1, 2], 4] } };
   dbcl.update(updateDoc12, { no: 12 });
   actual = dbcl.find({ no: 12 });
   commCompareResults(actual, [{ no: 12, a: [[1, 2], 4] }]);

   var updateDoc13 = { $saveMax: { a: { $regex: ".+" } } };
   dbcl.update(updateDoc13, { no: 13 });
   actual = dbcl.find({ no: 13 });
   commCompareResults(actual, [{ no: 13, a: { $regex: ".+" } }]);

   var updateDoc14 = { $saveMax: { a: { $field: "b" } } };
   dbcl.update(updateDoc14, { no: 18 });
   actual = dbcl.find({ no: 18 });
   commCompareResults(actual, [{ no: 18, a: 200, b: 200, c: 100 }]);

   var updateDoc15 = { $saveMax: { a: { $field: "c" } } };
   dbcl.update(updateDoc15, { no: 19 });
   actual = dbcl.find({ no: 19 });
   commCompareResults(actual, [{ no: 19, a: "c", b: "b", c: "c" }]);

   var updateDoc16 = { $saveMax: { a: { $field: "b" } } };
   dbcl.update(updateDoc16, { no: 20 });
   actual = dbcl.find({ no: 20 });
   commCompareResults(actual, [{ no: 20, a: true, b: true, c: false }]);

   // if value is less than the original value, the original value will not be updated
   var updateDoc17 = { $saveMax: { a: 0 } };
   dbcl.update(updateDoc17, { no: 1 });
   // check result
   actual = dbcl.find({ no: 1 });
   commCompareResults(actual, [{ no: 1, a: 2 }]);

   var updateDoc18 = { $saveMax: { a: "str" } };
   dbcl.update(updateDoc18, { no: 2 });
   actual = dbcl.find({ no: 2 });
   commCompareResults(actual, [{ no: 2, a: "ttr" }]);

   var updateDoc19 = { $saveMax: { a: { b: 0 } } };
   dbcl.update(updateDoc19, { no: 3 });
   actual = dbcl.find({ no: 3 });
   commCompareResults(actual, [{ no: 3, a: { b: 2 } }]);

   var updateDoc20 = { $saveMax: { a: ["str", 1] } };
   dbcl.update(updateDoc20, { no: 4 });
   actual = dbcl.find({ no: 4 });
   commCompareResults(actual, [{ no: 4, a: ["str", 3] }]);

   var updateDoc21 = { $saveMax: { a: { $field: "c" } } };
   dbcl.update(updateDoc21, { no: 18 });
   actual = dbcl.find({ no: 18 });
   commCompareResults(actual, [{ no: 18, a: 200, b: 200, c: 100 }]);

   var updateDoc22 = { $saveMax: { a: { $field: "b" } } };
   dbcl.update(updateDoc22, { no: 19 });
   actual = dbcl.find({ no: 19 });
   commCompareResults(actual, [{ no: 19, a: "c", b: "b", c: "c" }]);

   var updateDoc23 = { $saveMax: { a: { $field: "c" } } };
   dbcl.update(updateDoc23, { no: 20 });
   actual = dbcl.find({ no: 20 });
   commCompareResults(actual, [{ no: 20, a: true, b: true, c: false }]);

   commCheckLSN(db, testPara.srcGroupName);
   checkRecordConsistency(dbcl);
}
