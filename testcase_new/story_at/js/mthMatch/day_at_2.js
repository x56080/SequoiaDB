/***************************************************************************************************
 * @Description: $day作用于Date类型字段
 * @ATCaseID: day_at_2
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
 *     $day作用于Date类型字段
 * 测试步骤：
 *    1. $day作为选择符, 发起查询
 *    2. $day作为匹配符, 发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "day_at_2";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: { $date: "2021-02-01" } },
    { a: { $date: "2022-02-09" } },
    { a: { $date: "2023-03-11" } },
    { a: { $date: "2050-03-19" } },
    { a: { $date: "1999-03-1" } },
    { a: { $date: "0111-12-12" } },
    { a: { $date: "0999-12-20" } },
    { a: { $date: "0011-12-29" } },
    { a: { $date: "0099-12-12" } },
    { a: { $date: "2021-2-28" } },
    { a: { $date: "2020-2-29" } },
    { a: { $date: "0000-01-01" } },
    { a: { $date: "9999-12-31" } },
    { a: [{ $date: "9999-12-31" }, { $date: "2021-02-01" }] },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $day: 1 } });
  expResult = [
    { a: 1 },
    { a: 9 },
    { a: 11 },
    { a: 19 },
    { a: 1 },
    { a: 12 },
    { a: 20 },
    { a: 29 },
    { a: 12 },
    { a: 28 },
    { a: 29 },
    { a: 1 },
    { a: 31 },
    { a: [31, 1] },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $day: 1, $et: 31 } });
  expResult = [
    { a: { $date: "9999-12-31" } },
    { a: [{ $date: "9999-12-31" }, { $date: "2021-02-01" }] },
  ];
  commCompareResults(actResult, expResult);
}
