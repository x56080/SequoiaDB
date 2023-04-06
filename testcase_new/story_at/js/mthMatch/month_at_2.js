/***************************************************************************************************
 * @Description: $month作用于Date类型字段
 * @ATCaseID: month_at_2
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
 *     $month作用于Date类型字段
 * 测试步骤：
 *    1. $month作为选择符, 发起查询
 *    2. $month作为匹配符, 发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "month_at_2";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: { $date: "2021-01-01" } },
    { a: { $date: "2022-02-09" } },
    { a: { $date: "2023-03-11" } },
    { a: { $date: "2050-03-19" } },
    { a: { $date: "1999-04-1" } },
    { a: { $date: "0111-5-12" } },
    { a: { $date: "0999-6-20" } },
    { a: { $date: "0011-7-29" } },
    { a: { $date: "0099-8-12" } },
    { a: { $date: "2021-9-28" } },
    { a: { $date: "2020-10-29" } },
    { a: { $date: "0000-11-01" } },
    { a: { $date: "9999-12-31" } },
    { a: [{ $date: "9999-12-31" }, { $date: "2021-02-01" }] },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $month: 1 } });
  expResult = [
    { a: 1 },
    { a: 2 },
    { a: 3 },
    { a: 3 },
    { a: 4 },
    { a: 5 },
    { a: 6 },
    { a: 7 },
    { a: 8 },
    { a: 9 },
    { a: 10 },
    { a: 11 },
    { a: 12 },
    { a: [12, 2] },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $month: 1, $et: 12 } });
  expResult = [
    { a: { $date: "9999-12-31" } },
    { a: [{ $date: "9999-12-31" }, { $date: "2021-02-01" }] },
  ];
  commCompareResults(actResult, expResult);
}
