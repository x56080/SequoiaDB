/***************************************************************************************************
 * @Description: $substr功能测试
 * @ATCaseID: substr_at_1
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
 *    $substr功能性测试
 * 测试步骤：
 *    1. 准备字符串类型（单字节编码）记录和非字符串类型记录
 *    2. 测试$substr{ $substr : value }语法
 *    3. 测试$substr{ $substr : [ pos, len ] }语法
 *    4. 测试$substr作为匹配符{ $substr : value, "$": ...}语法
 *    5. 测试$substr作为匹配符{ $substr : [pos, len ], "$": ...}语法
 *    6. 校验非法参数
 * 期望结果：
 *    第2-5步输出期望子串结果
 *    第6步对外报错-6
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "substr_at_1";

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

  //  $substr as selector
  // 2.{  $substr : value }
  var actResult1 = dbcl.find({}, { a: { $substr: 3 } });
  var expResult1 = [
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

  commCompareResults(actResult1, expResult1);

  var actResult2 = dbcl.find({}, { a: { $substr: -3 } });
  var expResult2 = [
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
  commCompareResults(actResult2, expResult2);

  // 3.{  $substr : [ pos, len ] }
  var actResult3 = dbcl.find({}, { a: { $substr: [2, 3] } });
  var expResult3 = [
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
  commCompareResults(actResult3, expResult3);

  var actResult4 = dbcl.find({}, { a: { $substr: [-3, 3] } });
  var expResult4 = [
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
  commCompareResults(actResult4, expResult4);

  var actResult5 = dbcl.find({}, { a: { $substr: [-3, -1] } });
  var expResult5 = [
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
  commCompareResults(actResult5, expResult5);

  //  $substr as Matcher
  // 4.{  $substr : value, "$": ...}
  var actResult6 = dbcl.find({ a: { $substr: 2, $et: "Se" } });
  var expResult6 = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", 111] }];
  commCompareResults(actResult6, expResult6);

  // 5.{  $substr : [pos , len ], "$": ...}
  var actResult7 = dbcl.find({ a: { $substr: [0, 2], $et: "Se" } });
  var expResult7 = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", 111] }];
  commCompareResults(actResult7, expResult7);

  // 6.check arguments
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substr: true } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substr: null } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substr: [null, true] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substr: [1, 2, 3] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $substr: [null, true], et: "巨杉" } }).toArray();
  });
}
