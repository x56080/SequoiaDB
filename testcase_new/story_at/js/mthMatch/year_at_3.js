/***************************************************************************************************
 * @Description: $year作用于TimeStamp类型字段
 * @ATCaseID: year_at_3
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
 *     $year作用于TimeStamp类型字段
 * 测试步骤：
 *    1. $year作为选择符, 发起查询
 *    2. $year作为匹配符, 发起查询
 * 期望结果：
 *    期望结果与实际结果一致
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "year_at_3";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: { $timestamp: "1902-01-01-00:00:00.000000" } },
    { a: { $timestamp: "2037-12-31-23:59:59.999999" } },
    { a: { $timestamp: "2020-02-29-00:00:00.000000" } },
    { a: { $timestamp: "2019-02-28-00:00:00.000000" } },
    { a: { $timestamp: "1999-03-30-08.00.00.000000" } },
    { a: { $timestamp: "2000-03-30-00.00.59.000000" } },
    { a: { $timestamp: "2021-03-30-00.00.00.999999" } },
    { a: { $timestamp: "2025-03-30-00.00.59.999999" } },
    { a: { $timestamp: "2026-03-30-11.12.59.999999" } },
    {
      a: [
        { $timestamp: "2025-03-30-00.00.59.999999" },
        { $timestamp: "2026-03-30-11.12.59.999999" },
      ],
    },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $year: 1 } });
  expResult = [
    { a: 1902 },
    { a: 2037 },
    { a: 2020 },
    { a: 2019 },
    { a: 1999 },
    { a: 2000 },
    { a: 2021 },
    { a: 2025 },
    { a: 2026 },
    { a: [2025, 2026] },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({ a: { $year: 1, $et: 2026 } });
  expResult = [
    { a: { $timestamp: "2026-03-30-11.12.59.999999" } },
    {
      a: [
        { $timestamp: "2025-03-30-00.00.59.999999" },
        { $timestamp: "2026-03-30-11.12.59.999999" },
      ],
    },
  ];
  commCompareResults(actResult, expResult);
}
