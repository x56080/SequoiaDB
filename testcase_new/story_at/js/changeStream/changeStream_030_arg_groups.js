/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_030
 * @Author: Huang Youquan
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ========== =========================================================
 * 08/09/2023 Huang Youquan change stream
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    校验change stream groups参数
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "changeStream_030";

main(test);
function test() {

   assert.tryThrow(SDB_INVALIDARG, function () {
      db.watch(StreamToken(), { Groups: "aaa" });
   });

   assert.tryThrow(SDB_INVALIDARG, function () {
      db.watch(StreamToken(), { Groups: 1 });
   });

   assert.tryThrow(SDB_OPTION_NOT_SUPPORT, function () {
      db.watch(StreamToken(), { Groups: [] });
   });

   assert.tryThrow(SDB_OPTION_NOT_SUPPORT, function () {
      db.watch(StreamToken(), { Groups: ["db1", "db2"] });
   });

   assert.tryThrow(SDB_OPTION_NOT_SUPPORT, function () {
      db.watch(StreamToken());
   });
}