/***************************************************************************************************
 * @Description: $concat参数校验
 * @ATCaseID: concat_at_1
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/16/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *     $concat参数校验
 * 测试步骤：
 *    1. $concat参数为数组时，数组的大小不为2, 发起查询
 *    2. $concat参数为数组时, 第一个元素不是数字, 发起查询
 *    3. $concat参数为数组时, 第二个元素不是数组, 发起查询
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_1";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;

  // step 1
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [1] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [2, [1, 2], 3] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [], $et: "aaa" } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [1], $et: "aaa" } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [2, [1, 2], 3], $et: "aaa" } }).toArray();
  });

  // step 2
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: ["string", ["str"]] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [true, ["str"]] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [null, ["str"]] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: ["string", ["str"]], $et: "aaa" } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [true, ["str"]], $et: "aaa" } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [null, ["str"]], $et: "aaa" } }).toArray();
  });

  // step 3
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [1, "str"] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [1, true] } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $concat: [1, null] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [1, "str"], $et: "aaa" } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [1, true], $et: "aaa" } }).toArray();
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $concat: [1, null], $et: "aaa" } }).toArray();
  });
}
