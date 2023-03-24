/***************************************************************************************************
 * @Description: $substr作为选择符
 * @ATCaseID: substr_at_2
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/08/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *    $substr作为选择符测试
 * 测试步骤：
 *    1. 准备字符串类型（单字节编码）记录和非字符串类型记录
 *    2. 测试$substr{ $substr : value }语法
 *    3. 测试$substr{ $substr : [ pos, len ] }语法
 * 期望结果：
 *    输出期望子串结果
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "substr_at_2";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  // 1.prepare data
  var docs = [
    { a: "Sequoiadb" },
    { a: "se" },
    { a: ["Sequoiadb", 111] },
    { a: [111, true] },
    { a: 111 },
    { a: 2022.0211 },
    { a: true },
    { a: { $date: "2022-02-11T15:59:59.999Z" } },
    { a: { $timestamp: "2022-02-11T15:59:59.999Z" } },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  //  $substr as selector
  // 2.{  $substr : value }
  actResult = dbcl.find({}, { a: { $substr: 3 } });
  expResult = [
    { a: "Seq" },
    { a: "se" },
    { a: ["Seq", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $substr: -3 } });
  expResult = [
    { a: "adb" },
    { a: "" },
    { a: ["adb", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  // 3.{  $substr : [ pos, len ] }
  actResult = dbcl.find({}, { a: { $substr: [2, 3] } });
  expResult = [
    { a: "quo" },
    { a: "" },
    { a: ["quo", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $substr: [-3, 3] } });
  expResult = [
    { a: "adb" },
    { a: "" },
    { a: ["adb", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $substr: [-3, -1] } });
  expResult = [
    { a: "adb" },
    { a: "" },
    { a: ["adb", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);
}
