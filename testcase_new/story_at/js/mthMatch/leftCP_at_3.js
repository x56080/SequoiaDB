/***************************************************************************************************
 * @Description: $leftCP作为匹配符使用
 * @ATCaseID: leftCP_at_3
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
 *    $leftCP作为匹配符使用
 * 测试步骤：
 *    1. 准备字符串类型（不定长字节编码）记录和非字符串类型记录
 *    2. 测试$leftCP{ $leftCP : len }语法
 * 期望结果：
 *    输出期望子串结果
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "leftCP_at_3";

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

  // $leftCP as Matcher
  // 3.{ $leftCP : value, "$": ...}
  var actResult = dbcl.find({ a: { $leftCP: 2, $et: "巨杉" } });
  var expResult = [
    { a: "巨杉" },
    { a: "巨杉数据库" },
    { a: "巨杉Sequoiadb" },
    { a: ["Sequoiadb", "巨杉", 111] },
  ];
  commCompareResults(actResult, expResult);
}
