/***************************************************************************************************
 * @Description: $substrCP校验参数
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
 *    $substrCP校验参数
 * 测试步骤：
 *    1. 校验非法参数
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "substrCP_at_1";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;

  // 1.check arguments
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
    dbcl.find({}, { a: { $substrCP: [] } }).toArray();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcl.find({}, { a: { $substrCP: [1] } }).toArray();
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
