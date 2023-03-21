/***************************************************************************************************
 * @Description: $concat类型转换-int
 * @ATCaseID: concat_at_4
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
 *     $concat参数类型为int的使用
 * 测试步骤：
 *    1. $concat参数为int正数，发起查询
 *    2. $concat参数为int正数，发起查询
 *    3. $concat参数为int最大正数，发起查询
 *    4. $concat参数为int最小负数，发起查询
 * 期望结果：
 *   期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_4";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: 123 } });
  expResult = [{ a: "string123" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: -123 } });
  expResult = [{ a: "string-123" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: 2147483647 } });
  expResult = [{ a: "string2147483647" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: -2147483648 } });
  expResult = [{ a: "string-2147483648" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [123]] } });
  expResult = [{ a: "string123" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [-123]] } });
  expResult = [{ a: "string-123" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [2147483647]] } });
  expResult = [{ a: "string2147483647" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [-2147483648]] } });
  expResult = [{ a: "string-2147483648" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({ a: { $concat: 123, $et: "string123" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: -123, $et: "string-123" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: 2147483647, $et: "string2147483647" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: -2147483648, $et: "string-2147483648" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [123]], $et: "string123" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [-123]], $et: "string-123" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [2147483647]], $et: "string2147483647" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [-2147483648]], $et: "string-2147483648" } });
  commCompareResults(actResult, expResult);
}
