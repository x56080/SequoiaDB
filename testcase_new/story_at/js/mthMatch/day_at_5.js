/***************************************************************************************************
 * @Description: $day作用于number类型字段
 * @ATCaseID: day_at_5
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/30/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *     $day作用于number类型字段
 * 测试步骤：
 *    1. $day作为选择符, 发起查询
 *    2. $day作为匹配符, 发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "day_at_5";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: 0 },
    { a: 1 },
    { a: 2147483647 },
    { a: -1 },
    { a: -2147483648 },
    { a: { $numberLong: "3000000000" } },
    { a: { $numberLong: "9223372036854775807" } },
    { a: { $numberLong: "-3000000000" } },
    { a: { $numberLong: "-9223372036854775808" } },
    { a: 123.456 },
    { a: 1.7e308 },
    { a: -123.456 },
    { a: -1.7e308 },
    { a: { $decimal: "123.456" } },
    { a: { $decimal: "9223372036854775807" } },
    { a: { $decimal: "-9223372036854775808" } },
    { a: { $decimal: "9223372036854775808" } },
    { a: { $decimal: "-9223372036854775809" } },
    { a: [0, 1] },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $day: 1 } });
  expResult = [
    { a: 1 },
    { a: 1 },
    { a: 19 },
    { a: 1 },
    { a: 14 },
    { a: 5 },
    { a: 17 },
    { a: 27 },
    { a: 17 },
    { a: 1 },
    { a: 17 },
    { a: 1 },
    { a: 17 },
    { a: 1 },
    { a: 17 },
    { a: 17 },
    { a: null },
    { a: null },
    { a: [1, 1] },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $day: 1, $et: 5 } });
  expResult = [{ a: { $numberLong: "3000000000" } }];
  commCompareResults(actResult, expResult);
}
