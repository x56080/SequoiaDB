/***************************************************************************************************
 * @Description: $rightCP校验参数
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
 *    $rightCP校验参数
 * 测试步骤：
 *    1. 校验非法参数
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "rightCP_at_1";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;

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
