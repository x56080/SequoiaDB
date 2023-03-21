/***************************************************************************************************
 * @Description: $concat类型转换-bool
 * @ATCaseID: concat_at_8
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
 *     $concat参数类型为bool的使用
 * 测试步骤：
 *     1. $concat参数为true，发起查询
 *     2. $concat参数为负false，发起查询
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_8";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: true } });
  expResult = [{ a: "stringtrue" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: false } });
  expResult = [{ a: "stringfalse" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [true]] } });
  expResult = [{ a: "stringtrue" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [false]] } });
  expResult = [{ a: "stringfalse" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({ a: { $concat: [0, [true]], $et: "stringtrue" } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $concat: [0, [false]], $et: "stringfalse" } });
  commCompareResults(actResult, expResult);
}
