/***************************************************************************************************
 * @Description: $month作用于TimeStamp类型字段
 * @ATCaseID: month_at_3
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
 *     $month作用于TimeStamp类型字段
 * 测试步骤：
 *    1. $month作为选择符, 发起查询
 *    2. $month作为匹配符, 发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "month_at_3";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: { $timestamp: "1902-01-01-00:00:00.000000" } },
    { a: { $timestamp: "2037-12-31-23:59:59.999999" } },
    { a: { $timestamp: "2020-02-29-00:00:00.000000" } },
    { a: { $timestamp: "2019-02-28-00:00:00.000000" } },
    { a: { $timestamp: "1999-03-30-08.00.00.000000" } },
    { a: { $timestamp: "2000-04-10-00.00.59.000000" } },
    { a: { $timestamp: "2021-05-19-00.00.00.999999" } },
    { a: { $timestamp: "2025-06-25-00.00.59.999999" } },
    { a: { $timestamp: "2026-07-30-11.12.59.999999" } },
    {
      a: [
        { $timestamp: "2025-06-25-00.00.59.999999" },
        { $timestamp: "2026-07-30-11.12.59.999999" },
      ],
    },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $month: 1 } });
  expResult = [
    { a: 1 },
    { a: 12 },
    { a: 2 },
    { a: 2 },
    { a: 3 },
    { a: 4 },
    { a: 5 },
    { a: 6 },
    { a: 7 },
    { a: [6, 7] },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $month: 1, $et: 6 } });
  expResult = [
    { a: { $timestamp: "2025-06-25-00.00.59.999999" } },
    {
      a: [
        { $timestamp: "2025-06-25-00.00.59.999999" },
        { $timestamp: "2026-07-30-11.12.59.999999" },
      ],
    },
  ];
  commCompareResults(actResult, expResult);
}
