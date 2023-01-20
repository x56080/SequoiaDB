/***************************************************************************************************
 * @Description: 外部模式管理：重命名字段
 * @ATCaseID: schema_18
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Rename column
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.创建外部模式，未与集合绑定，重命名外部模式的字段
 *    2.创建外部模式，与集合绑定，重命名外部模式的字段
 * 测试步骤：
 *    1.创建外部模式
 *    2.根据场景是否绑定集合
 *    3.重命名外部模式字段
 * 期望结果：
 *    均重命名成功
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_18";
  var schemaName1 = clName + "_1";
  commClearLegacySchema(db, schemaName1);
  var columnDef = { Type: "int32", WriteDefault: 10, ReadDefault: 5 };
  var schema1 = db.createSchema(schemaName1, {
    field1: columnDef,
  });

  schema1.renameColumn("field1", "field2");
  checkColumnDef(db, schemaName1, "field2", columnDef);
  db.dropSchema(schemaName1);

  var cl = commCreateCL(db, COMMCSNAME, clName, { EnableInfoSchema: true });
  var schemaName2 = clName + "_2";
  commClearLegacySchema(db, schemaName2);
  var schema2 = db.createSchema(schemaName2, {
    field1: columnDef,
  });
  cl.addSchema(schemaName2);
  schema2.renameColumn("field1", "field2");
  checkColumnDef(db, schemaName2, "field2", columnDef);
  commDropCL(db, COMMCSNAME, clName);
}
