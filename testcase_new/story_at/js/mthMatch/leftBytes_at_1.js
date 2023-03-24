/***************************************************************************************************
 * @Description: $leftBytes参数校验
 * @ATCaseID: leftBytes_at_1
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
 *     $leftBytes功能性测试
 * 测试步骤：
 *    1. 校验非法参数
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "leftBytes_at_1";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;

  // 1.check arguments
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $leftBytes: true } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $leftBytes: null } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $leftBytes: [1, 2] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $leftBytes: [1, null] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $leftBytes: [null, true] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $leftBytes: [1, 2, 3] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $leftBytes: [null, true], et: "Se" } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({ a: { $leftBytes: [1, 2], et: "Se" } }).toArray();
  });
}
