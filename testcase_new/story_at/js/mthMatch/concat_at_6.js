/***************************************************************************************************
 * @Description: $concat类型转换-double
 * @ATCaseID: concat_at_6
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
 *     $concat参数类型为double的使用
 * 测试步骤：
 *    1. $concat参数为正double，发起查询
 *    2. $concat参数为负double，发起查询
 *    3. $concat参数为正double最大值，发起查询
 *    4. $concat参数为负double最小值，发起查询
 * 期望结果：
 *    期望结果与实际结果一致

 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_6";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: 123.45 } });
  expResult = [{ a: "string123.45" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: -123.456 } });
  expResult = [{ a: "string-123.456" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: 1.7e308 } });
  expResult = [{ a: "string1.7e+308" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: -1.7e308 } });
  expResult = [{ a: "string-1.7e+308" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [123.45]] } });
  expResult = [{ a: "string123.45" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [-123.456]] } });
  expResult = [{ a: "string-123.456" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [1.7e308]] } });
  expResult = [{ a: "string1.7e+308" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [-1.7e308]] } });
  expResult = [{ a: "string-1.7e+308" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({ a: { $concat: [0, [123.45]], $et: "string123.45" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [-123.456]], $et: "string-123.456" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [1.7e308]], $et: "string1.7e+308" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [-1.7e308]], $et: "string-1.7e+308" } });
  commCompareResults(actResult, expResult);
}
