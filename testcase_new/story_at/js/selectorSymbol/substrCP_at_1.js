/***************************************************************************************************
 * @Description: $substrCP功能测试
 * @ATCaseID: substrCP_at_1
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
 *    $substrCP功能性测试
 * 测试步骤：
 *    1. 准备字符串类型（不定长字节编码）记录和非字符串类型记录
 *    2. 测试$substrCP作为选择符{ $substrCP : value }语法
 *    3. 测试$substrCP作为选择符{ $substrCP : [ pos, len ] }语法
 *    4. 测试$substrCP作为匹配符{ $substrCP : value, "$": ...}语法
 *    5. 测试$substrCP作为匹配符{ $substrCP : [pos, len ], "$": ...}语法
 *    6. 校验非法参数
 * 期望结果：
 *    第2-5步输出期望子串结果
 *    第6步对外报错-6
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "substrCP_at_1";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  // 1.prepare data
  var docs = [
    { a: "Sequoiadb" },
    { a: "巨杉" },
    { a: "巨杉数据库" },
    { a: "巨杉Sequoiadb" },
    { a: "d巨杉dba" },
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

  // $substrCP as selector
  // 2.{ $substrCP : value }
  var actResult1 = dbcl.find({}, { a: { $substrCP: 3 } });
  var expResult1 = [
    { a: "Seq" },
    { a: "巨杉" },
    { a: "巨杉数" },
    { a: "巨杉S" },
    { a: "d巨杉" },
    { a: ["Seq", "巨杉", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "𐍈𝄞𠮷" },
    { a: "éè𐍈" },
  ];

  commCompareResults(actResult1, expResult1);

  var actResult2 = dbcl.find({}, { a: { $substrCP: -3 } });
  var expResult2 = [
    { a: "adb" },
    { a: "" },
    { a: "数据库" },
    { a: "adb" },
    { a: "dba" },
    { a: ["adb", "", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "𠮷éè" },
    { a: "𐍈𝄞𠮷" },
  ];
  commCompareResults(actResult2, expResult2);

  // 3.{ $substrCP : [ pos, len ] }
  var actResult3 = dbcl.find({}, { a: { $substrCP: [2, 3] } });
  var expResult3 = [
    { a: "quo" },
    { a: "" },
    { a: "数据库" },
    { a: "Seq" },
    { a: "杉db" },
    { a: ["quo", "", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "𠮷éè" },
    { a: "𐍈𝄞𠮷" },
  ];
  commCompareResults(actResult3, expResult3);

  var actResult4 = dbcl.find({}, { a: { $substrCP: [-3, 3] } });
  var expResult4 = [
    { a: "adb" },
    { a: "" },
    { a: "数据库" },
    { a: "adb" },
    { a: "dba" },
    { a: ["adb", "", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "𠮷éè" },
    { a: "𐍈𝄞𠮷" },
  ];
  commCompareResults(actResult4, expResult4);

  var actResult5 = dbcl.find({}, { a: { $substrCP: [-3, -1] } });
  var expResult5 = [
    { a: "adb" },
    { a: "" },
    { a: "数据库" },
    { a: "adb" },
    { a: "dba" },
    { a: ["adb", "", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: "𠮷éè" },
    { a: "𐍈𝄞𠮷" },
  ];
  commCompareResults(actResult5, expResult5);

  // $substrCP as Matcher
  // 4.{ $substrCP : value, "$": ...}
  var actResult6 = dbcl.find({ a: { $substrCP: 2, $et: "巨杉" } });
  var expResult6 = [
    { a: "巨杉" },
    { a: "巨杉数据库" },
    { a: "巨杉Sequoiadb" },
    { a: ["Sequoiadb", "巨杉", 111] },
  ];
  commCompareResults(actResult6, expResult6);

  // 5.{ $substrCP : [pos , len ], "$": ...}
  var actResult7 = dbcl.find({ a: { $substrCP: [0, 2], $et: "巨杉" } });
  var expResult7 = [
    { a: "巨杉" },
    { a: "巨杉数据库" },
    { a: "巨杉Sequoiadb" },
    { a: ["Sequoiadb", "巨杉", 111] },
  ];
  commCompareResults(actResult7, expResult7);

  // 6.check arguments
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substrCP: true } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substrCP: null } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substrCP: [1, true] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substrCP: [null, true] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substrCP: [1, 2, 3] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $substrCP: [null, true], et: "巨杉" } }).toArray();
  });
}
