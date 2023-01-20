/***************************************************************************************************
 * @Description: 索引操作：删除索引
 * @ATCaseID: schema_17
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Drop index with schema
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.索引创建在外部模式已有字段
 *    2.索引创建在外部模式新增字段
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建集合并绑定外部模式
 *    3.索引创建在已有字段
 *    4.插入数据
 *    5.删除索引
 * 期望结果：
 *    索引删除成功
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_17";
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    int: { Type: "int32", WriteDefault: 10, ReadDefault: 5 },
  });
  var cl = commCreateCL(db, COMMCSNAME, clName, { EnableInfoSchema: true });
  cl.addSchema(schemaName);
  cl.createIndex("index1", { int: 1 });
  insertData(cl);

  cl.dropIndex("index1");
  commDropCL(db, COMMCSNAME, clName);
}
