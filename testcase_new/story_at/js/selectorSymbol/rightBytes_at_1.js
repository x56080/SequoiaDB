/***************************************************************************************************
 * @Description: $rightBytes功能测试
 * @ATCaseID: rightBytes_at_1
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
 *    $rightBytes功能性测试
 * 测试步骤：
 *    1. 准备字符串类型（单字节编码）记录和非字符串类型记录
 *    2. 测试$rightBytes作为选择符{ $rightBytes : len }语法
 *    3. 测试$rightBytes作为匹配符{ $rightBytes : len, "$": ...}语法
 *    4. 校验非法参数
 * 期望结果：
 *    第2-3步输出期望子串结果
 *    第4步对外报错-6
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "rightBytes_at_1";

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

  //$rightBytes as selector
  // 2.{ $rightBytes : value }
  var actResult1 = dbcl.find({}, { a: { $rightBytes: 3 } });
  var expResult1 = [
    { a: "adb" },
    { a: "ab" },
    { a: ["adb", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult1, expResult1);

  var actResult2 = dbcl.find({}, { a: { $rightBytes: -3 } });
  var expResult2 = [
    { a: "" },
    { a: "" },
    { a: ["", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult2, expResult2);

  var actResult3 = dbcl.find({}, { a: { $rightBytes: 0 } });
  var expResult3 = [
    { a: "" },
    { a: "" },
    { a: ["", null] },
    { a: [null, null] },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult3, expResult3);

  // $rightBytes as Matcher
  // 3.{ $rightBytes : value, "$": ...}
  var actResult4 = dbcl.find({ a: { $rightBytes: 2, $et: "db" } });
  var expResult4 = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", 111] }];
  commCompareResults(actResult4, expResult4);

  // 4.check arguments
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightBytes: true } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightBytes: null } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightBytes: [1, 2] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightBytes: [1, null] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightBytes: [null, true] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $rightBytes: [1, 2, 3] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $rightBytes: [null, true], et: "Se" } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $rightBytes: [1, 2], et: "Se" } }).toArray();
  });
}
