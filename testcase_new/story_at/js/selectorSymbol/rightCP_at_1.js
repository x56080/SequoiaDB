/***************************************************************************************************
 * @Description: $rightCP功能测试
 * @ATCaseID: rightCP_at_1
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
 *    $rightCP功能性测试
 * 测试步骤：
 *    1. 准备字符串类型（不定长字节编码）记录和非字符串类型记录
 *    2. 测试$rightCP作为选择符{ $rightCP : len }语法
 *    3. 测试$rightCP作为匹配符{ $rightCP: len, "$": ...}语法
 *    4. 校验非法参数
 * 期望结果：
 *    第2-3步输出期望子串结果
 *    第4步对外报错-6
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "rightCP_at_1";

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

  //$rightCP as selector
  // 2.{ $rightCP : value }
  var actResult1 = dbcl.find({}, { a: { $rightCP: 3 } });
  var expResult1 = [
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
  commCompareResults(actResult1, expResult1);

  var actResult2 = dbcl.find({}, { a: { $rightCP: -3 } });
  var expResult2 = [
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
  commCompareResults(actResult2, expResult2);

  var actResult3 = dbcl.find({}, { a: { $rightCP: 0 } });
  var expResult3 = [
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
  commCompareResults(actResult3, expResult3);

  // $rightCP as Matcher
  // 3.{ $rightCP : value, "$": ...}
  var actResult4 = dbcl.find({ a: { $rightCP: 2, $et: "巨杉" } });
  var expResult4 = [{ a: "巨杉" }, { a: ["Sequoiadb", "巨杉", 111] }];
  commCompareResults(actResult4, expResult4);

  // 4.check arguments
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightCP: true } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightCP: null } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightCP: [1, 2] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightCP: [1, null] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightCP: [null, true] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightCP: [1, 2, 3] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $rightCP: [null, true], et: "巨杉" } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $rightCP: [1, 2], et: "巨杉" } }).toArray();
  });
}
