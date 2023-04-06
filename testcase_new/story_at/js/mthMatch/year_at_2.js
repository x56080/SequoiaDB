/***************************************************************************************************
 * @Description: $year作用于Date类型字段
 * @ATCaseID: year_at_2
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
 *     $year作用于Date类型字段
 * 测试步骤：
 *    1. $year作为选择符, 发起查询
 *    2. $year作为匹配符, 发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "year_at_2";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: { $date: "2021-02-01" } },
    { a: { $date: "2022-02-09" } },
    { a: { $date: "2023-03-11" } },
    { a: { $date: "2050-03-11" } },
    { a: { $date: "1999-03-11" } },
    { a: { $date: "0111-12-12" } },
    { a: { $date: "0999-12-12" } },
    { a: { $date: "0011-12-12" } },
    { a: { $date: "0099-12-12" } },
    { a: { $date: "0001-9-12" } },
    { a: { $date: "0009-2-12" } },
    { a: { $date: "0000-01-01" } },
    { a: { $date: "9999-12-31" } },
    { a: [{ $date: "9999-12-31" }, { $date: "2021-02-01" }] },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $year: 1 } });
  expResult = [
    { a: 2021 },
    { a: 2022 },
    { a: 2023 },
    { a: 2050 },
    { a: 1999 },
    { a: 111 },
    { a: 999 },
    { a: 11 },
    { a: 99 },
    { a: 1 },
    { a: 9 },
    { a: 0 },
    { a: 9999 },
    { a: [9999, 2021] },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $year: 1, $et: 9999 } });
  expResult = [
    { a: { $date: "9999-12-31" } },
    { a: [{ $date: "9999-12-31" }, { $date: "2021-02-01" }] },
  ];
  commCompareResults(actResult, expResult);
}
