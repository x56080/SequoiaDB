/***************************************************************************************************
 * @Description: dropCL()接口Comment参数校验
 * @ATCaseID: recycle_at_3
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 04/14/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *      dropCL()接口Comment参数校验
 * 测试步骤：
 *      1.Comment无效参数校验
 *      2.Comment长度校验
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/

main(test);
function test() {
  var csName = "recycle_at_3_cs";
  var clName = "recycle_at_3_cl";
  assert.equal(db.getRecycleBin().getDetail().toObj().Enable, true);
  var dbcs = commCreateCS(db, csName);
  dbcs.createCL(clName, { ShardingKey: { a: 1 } });
  // 无效参数值校验
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcs.dropCL(clName, { Comment: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcs.dropCL(clName, { Comment: null });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcs.dropCL(clName, { Comment: 0 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcs.dropCL(clName, { Comment: 1 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcs.dropCL(clName, { Comment: -1 });
  });

  // 长度校验
  var arr = new Array(128 * 1024 + 1);
  var Comment = arr.join("a");
  assert.tryThrow(SDB_INVALIDARG, function () {
    dbcs.dropCL(csName, { Comment: Comment });
  });

  commDropCS(db, csName);
  db.getRecycleBin().dropAll();
}
