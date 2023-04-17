/***************************************************************************************************
 * @Description: truncate()接口Comment参数校验
 * @ATCaseID: recycle_at_5
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
 *      truncate()接口Comment参数校验
 * 测试步骤：
 *       1.Comment无效参数校验
 *       2.Comment长度校验
 * 期望结果：
 *    对外报错-6
 *
 **************************************************************************************************/

testConf.csName = CHANGEDPREFIX + "recycle_at_5";
testConf.clName = COMMCLNAME + "recycle_at_5";

main(test);
function test() {
  assert.equal(db.getRecycleBin().getDetail().toObj().Enable, true);

  var cl = testPara.testCL;
  cl.insert({ a: 1 });

  // 无效参数值校验
  assert.tryThrow(SDB_INVALIDARG, function () {
    cl.truncate({ Comment: 0 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    cl.truncate({ Comment: 1 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    cl.truncate({ Comment: -1 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    cl.truncate({ Comment: null });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    cl.truncate({ Comment: true });
  });

  // 长度校验
  var arr = new Array(128 * 1024 + 1);
  var Comment = arr.join("a");
  assert.tryThrow(SDB_INVALIDARG, function () {
    cl.truncate({ Comment: Comment });
  });
}
