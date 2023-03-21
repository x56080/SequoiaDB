/***************************************************************************************************
 * @Description: $concat类型转换-($binary、$regex)等不能转换的类型
 * @ATCaseID: concat_at_14
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
 *     $concat参数类型为不能转换类型的使用
 * 测试步骤：
 *     1. $concat参数为binary，发起查询
 *     2. $concat参数为regex，发起查询
 *     3. $concat参数为null，发起查询
 *     4. $concat参数数组且里面有部分不能转换的类型，发起查询
 * 期望结果：
 *     返回null
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_14";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult = [{ a: null}];

  actResult = dbcl.find({}, { a: { $concat: { $binary : "aGVsbG8gd29ybGQ=", $type : "1" } } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: { $regex : "^张", $options : "i" }} });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: null } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: {$minKey: 1 }} });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: {$maxKey: 1 } } });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [-1, [{ $timestamp: "2012-01-01-13.14.26.124233" }, null]]} });
  commCompareResults(actResult, expResult);

}
