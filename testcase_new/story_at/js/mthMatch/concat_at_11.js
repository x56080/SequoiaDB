/***************************************************************************************************
 * @Description: $concat类型转换-timestamp
 * @ATCaseID: concat_at_11
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
 *     $concat参数类型为timestamp的使用
 * 测试步骤：
 *    1. $concat参数为timestamp，发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_11";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: { $timestamp: "2012-01-01-13.14.26.124233" } } });
  expResult = [{ a: "string2012-01-01-13.14.26.124233" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find(
    {},
    { a: { $concat: [-1, [{ $timestamp: "2012-01-01-13.14.26.124233" }]] } }
  );
  expResult = [{ a: "2012-01-01-13.14.26.124233string" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({
    a: {
      $concat: [0, [{ $timestamp: "2012-01-01-13.14.26.124233" }]],
      $et: "string2012-01-01-13.14.26.124233",
    },
  });
  commCompareResults(actResult, expResult);
}
