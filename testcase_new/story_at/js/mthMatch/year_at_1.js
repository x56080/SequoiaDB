/***************************************************************************************************
 * @Description: $year参数校验
 * @ATCaseID: year_at_1
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
 *     $year参数校验
 * 测试步骤：
 *    1. $year参数为数字时，不为1, 发起查询
 *    2. $year参数为其他类型时, 发起查询
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "year_at_1";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: -1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: 0 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: 2 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: [1, 2] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: true } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: null } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: "" } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: "123" } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $year: -1, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $year: 0, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: 2, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: [1, 2], $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: true, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: null, $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: "", $et: 1 } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $year: "123", $et: 1 } }).toArray();
  });
}
