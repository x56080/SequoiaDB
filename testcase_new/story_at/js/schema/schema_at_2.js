/***************************************************************************************************
 * @Description: 内部模式管理：对已经存在的集合开启内部模式
 * @ATCaseID: schema_2
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Enable internal schema for existing collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    内部模式管理：对已经存在的集合开启内部模式
 * 测试步骤：
 *    1.创建集合，指定不开启内部模式
 *    2.插入数据
 *    3.使用alter接口开启内部模式
 * 期望结果：
 *    操作成功，查看集合元数据，属性中标识已开启内部模式
 *
 **************************************************************************************************/
testConf.clName = "schema_2";
testConf.clOpt = { EnableInfoSchema: false };
main(test);

function test() {
  var cl = testPara.testCL;
  insertData(cl);
  cl.alter({ EnableInfoSchema: true });
  checkIfInfoSchemaEnabled(db, testConf.csName, testConf.clName);
}