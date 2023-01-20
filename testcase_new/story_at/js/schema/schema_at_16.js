/***************************************************************************************************
 * @Description: 索引操作：创建索引
 * @ATCaseID: schema_16
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Create index with schema
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
 *    4.外部模式新增字段
 *    5.索引创建在新增字段
 * 期望结果：
 *    索引创建成功
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_16";
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    int: { Type: "int32", WriteDefault: 10, ReadDefault: 5 },
  });
  var cl = commCreateCL(db, COMMCSNAME, clName, { EnableInfoSchema: true });
  cl.addSchema(schemaName);
  cl.createIndex("index1", { int: 1 });
  insertData(cl);
  cl.find({int:{ $gt: 100 }});

  schema.addColumn("new_column", { Type: "double", WriteDefault: 10.5, ReadDefault: 5.5 });
  cl.createIndex("index2", { new_column: 1 });
  commDropCL(db, COMMCSNAME, clName);
}
