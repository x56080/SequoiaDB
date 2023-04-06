/***************************************************************************************************
 * @Description: $day参数校验
 * @ATCaseID: day_at_1
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
 *     $day参数校验
 * 测试步骤：
 *    1. $day参数为数字时，不为1, 发起查询
 *    2. $day参数为其他类型时, 发起查询
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "day_at_1";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: -1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: 0 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: 2 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: [1, 2] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: true } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: null } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: "" } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: "123" } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $day: -1, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $day: 0, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: 2, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: [1, 2], $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: true, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: null, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: "", $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $day: "123", $et: 1 } }).toArray();
  });
}
