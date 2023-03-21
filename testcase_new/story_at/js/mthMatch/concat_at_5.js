/***************************************************************************************************
 * @Description: $concat类型转换-long
 * @ATCaseID: concat_at_5
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/16/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *     $concat参数类型为long的使用
 * 测试步骤：
 *    1. $concat参数为long正数，发起查询
 *    2. $concat参数为long正数，发起查询
 *    3. $concat参数为long最大正数，发起查询
 *    4. $concat参数为long最小负数，发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_5";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: 3000000000 } });
  expResult = [{ a: "string3e+09" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: -3000000000 } });
  expResult = [{ a: "string-3e+09" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: 9223372036854775807 } });
  expResult = [{ a: "string9.22337e+18" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: -9223372036854775808 } });
  expResult = [{ a: "string-9.22337e+18" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [3000000000]] } });
  expResult = [{ a: "string3e+09" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [-3000000000]] } });
  expResult = [{ a: "string-3e+09" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [9223372036854775807]] } });
  expResult = [{ a: "string9.22337e+18" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [-9223372036854775808]] } });
  expResult = [{ a: "string-9.22337e+18" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({ a: { $concat: 3000000000, $et: "string3e+09" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: -3000000000, $et: "string-3e+09" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: 9223372036854775807, $et: "string9.22337e+18" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: -9223372036854775808, $et: "string-9.22337e+18" } });
  commCompareResults(actResult, expResult);
}
