/***************************************************************************************************
 * @Description: $concat类型转换-decimal
 * @ATCaseID: concat_at_7
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
 *     $concat参数类型为decimal的使用
 * 测试步骤：
 *    1. $concat参数为正decimal，发起查询
 *    2. $concat参数为负decimal，发起查询
 * 期望结果：
 *   期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_7";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: { $decimal: "123.4563123131313131311111111" } } });
  expResult = [{ a: "string123.4563123131313131311111111" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: { $decimal: "-123.4563123131313131311111111" } } });
  expResult = [{ a: "string-123.4563123131313131311111111" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find(
    {},
    { a: { $concat: [0, [{ $decimal: "123.4563123131313131311111111" }]] } }
  );
  expResult = [{ a: "string123.4563123131313131311111111" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find(
    {},
    { a: { $concat: [0, [{ $decimal: "-123.4563123131313131311111111" }]] } }
  );
  expResult = [{ a: "string-123.4563123131313131311111111" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({
    a: {
      $concat: [0, [{ $decimal: "123.4563123131313131311111111" }]],
      $et: "string123.4563123131313131311111111",
    },
  });
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({
    a: {
      $concat: [0, [{ $decimal: "-123.4563123131313131311111111" }]],
      $et: "string-123.4563123131313131311111111",
    },
  });
  commCompareResults(actResult, expResult);
}
