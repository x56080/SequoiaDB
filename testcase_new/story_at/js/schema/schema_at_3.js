/***************************************************************************************************
 * @Description: 内部模式管理：尝试关闭集合的内部模式
 * @ATCaseID: schema_3
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Try to disable internal schema for existing collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    内部模式管理：尝试关闭集合的内部模式
 * 测试步骤：
 *    1.创建集合，指定开启内部模式
 *    2.使用alter接口关闭内部模式
 * 期望结果：
 *    报错，集合内部模式开启后不能关闭
 *
 **************************************************************************************************/
testConf.clName = "schema_3";
testConf.clOpt = { EnableInfoSchema: true };
main(test);

function test() {
  var cl = testPara.testCL;
  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function () {
    cl.alter({ EnableInfoSchema: false });
  });
}