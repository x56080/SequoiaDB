/***************************************************************************************************
 * @Description: $concat类型转换-Date
 * @ATCaseID: concat_at_10
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
 *     $concat参数类型为Date的使用
 * 测试步骤：
 *    1. $concat参数为Date，发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_10";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: { $date: "2012-01-01" } } });
  expResult = [{ a: "string2012-01-01" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [-1, [{ $date: "2012-01-01" }]] } });
  expResult = [{ a: "2012-01-01string" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({
    a: {
      $concat: [0, [{ $date: "2012-01-01" }]],
      $et: "string2012-01-01",
    },
  });
  commCompareResults(actResult, expResult);
}
