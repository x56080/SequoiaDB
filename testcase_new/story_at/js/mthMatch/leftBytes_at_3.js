/***************************************************************************************************
 * @Description: $leftBytes作为匹配符使用
 * @ATCaseID: leftBytes_at_3
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
 *     $leftBytes作为匹配符使用
 * 测试步骤：
 *    1. 准备字符串类型（单字节编码）记录和非字符串类型记录
 *    2. 测试$leftBytes{ $leftBytes : len }语法
 * 期望结果：
 *    输出期望子串结果
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "leftBytes_at_3";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  // 1.prepare data
  var docs = [
    { a: "Sequoiadb" },
    { a: "ab" },
    { a: ["Sequoiadb", 111] },
    { a: [111, true] },
    { a: 111 },
    { a: 2022.0211 },
    { a: true },
    { a: { $date: "2022-02-11T15:59:59.999Z" } },
    { a: { $timestamp: "2022-02-11T15:59:59.999Z" } },
  ];
  dbcl.insert(docs);

  // $leftBytes as Matcher
  // 2.{ $leftBytes : value, "$": ...}
  var actResult = dbcl.find({ a: { $leftBytes: 2, $et: "Se" } });
  var expResult = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", 111] }];
  commCompareResults(actResult, expResult);
}
