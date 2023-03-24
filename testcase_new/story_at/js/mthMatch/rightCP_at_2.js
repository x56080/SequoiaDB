/***************************************************************************************************
 * @Description: $rightCP作为选择符使用
 * @ATCaseID: rightCP_at_2
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
 *    $rightCP作为选择符使用
 * 测试步骤：
 *    1. 准备字符串类型（不定长字节编码）记录和非字符串类型记录
 *    2. 测试$rightCP{ $rightCP : len }语法
 *    3. 校验非法参数
 * 期望结果：
 *    输出期望子串结果
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "rightCP_at_2";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  // 1.prepare data
  var docs = [
    { a: "Sequoiadb" },
    { a: "巨杉" },
    { a: "巨杉数据库" },
    { a: "巨杉Sequoiadb" },
    { a: ["Sequoiadb", "巨杉", 111] },
    { a: [111, true] },
    { a: 111 },
    { a: 2022.0211 },
    { a: true },
    { a: { $date: "2022-02-11T15:59:59.999Z" } },
    { a: { $timestamp: "2022-02-11T15:59:59.999Z" } },
    { a: "𐍈𝄞𠮷éè" },
    { a: "éè𐍈𝄞𠮷" },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  //$rightCP as selector
  // 2.{ $rightCP : value }
  actResult = dbcl.find({}, { a: { $rightCP: 3 } });
  expResult = [
    { a: "adb" },
    { a: "巨杉" },
    { a: "数据库" },
    { a: "adb" },
    { a: ["adb", "巨杉", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "𠮷éè" },
    { a: "𐍈𝄞𠮷" },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $rightCP: -3 } });
  expResult = [
    { a: "" },
    { a: "" },
    { a: "" },
    { a: "" },
    { a: ["", "", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "" },
    { a: "" },
  ];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $rightCP: 0 } });
  expResult = [
    { a: "" },
    { a: "" },
    { a: "" },
    { a: "" },
    { a: ["", "", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "" },
    { a: "" },
  ];
  commCompareResults(actResult, expResult);
}
