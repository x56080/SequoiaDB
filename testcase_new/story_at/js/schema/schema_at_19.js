/***************************************************************************************************
 * @Description:  内部模式管理:查看数据库分区集合的内部模式
 * @ATCaseID: schema_19
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/17/2023 Zhou Hongye Check the internal schema of sharding collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.查看数据库分区集合的内部模式
 * 测试步骤：
 *    1.创建数据库分区的集合
 *    2.绑定外部模式
 *    3.插入数据
 *    4.检查内部模式
 * 期望结果：
 *    内部模式各字段的默认值符合预期
 *
 **************************************************************************************************/
testConf.clName = "schema_19";
testConf.clOpt = { EnableInfoSchema: true };
main(test);

function test() {
  
  var cl = testPara.testCL;
  var schemaName = testConf.clName + "_1";
  commClearLegacySchema(db, schemaName);
  var columnsDef = {
   a: { Type: "int32", ReadDefault: 5 },
   b: { Type: "double", WriteDefault: 10.5 },
 };
  db.createSchema(schemaName, columnsDef);
  cl.addSchema(schemaName);
  for (var i = 0; i < 10000; ++i) {
    cl.insert({ rid: i });
  }
  cl.enableSharding({ ShardingKey: { rid: 1 }, AutoSplit: true });
  checkInternalSchema(db, COMMCSNAME, testConf.clName, columnsDef);
}
