/***************************************************************************************************
 * @Description: truncate()接口使用Comment参数
 * @ATCaseID: recycle_at_6
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
 *      truncate()接口使用Comment参数
 * 测试步骤：
 *     1. 创建CL
 *     2. 带Comment参数truncate()
 *     3. 检查回收站列表Comment字段
 *     4. 检查回收站快照Comment字段
 * 期望结果：
 *    预期结果与期待结果相同
 *
 **************************************************************************************************/
main(test);
function test() {
  var csName = "recycle_at_6_cs";
  var clName = "recycle_at_6_cl";
  assert.equal(db.getRecycleBin().getDetail().toObj().Enable, true);
  var recycle = db.getRecycleBin();
  recycle.dropAll();

  // 随机生成字符串
  var testString = generateRandomString();

  var dbcs = db.createCS(csName);
  var cl = dbcs.createCL(clName, { ShardingKey: { a: 1 } });
  cl.insert({ a: 1 });
  cl.truncate({ Comment: testString });
  //回收站列表
  var comment = "";
  var cursor = recycle.list();
  if (cursor.next()) {
    comment = cursor.current().toObj().Comment;
  }
  assert.equal(comment, testString);
  // 回收站快照
  cursor = recycle.snapshot();
  if (cursor.next()) {
    comment = cursor.current().toObj().Comment;
  }
  assert.equal(comment, testString);
  commDropCS(db, csName);
  recycle.dropAll();
}