/***************************************************************************************************
 * @Description: 外部模式管理：修改与集合绑定的外部模式——增加字段场景
 * @ATCaseID: schema_7
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Modify the external schema that is bound to a collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.增加字段，不指定默认值
 *    2.增加字段，指定默认值
 * 测试步骤：
 *    1.创建外部模式
 *    2.在外部模式上添加字段
 * 期望结果：
 *    1.通过快照查看schema元数据中完成了对应修改
 *    2.ZHY TODO 影响数据节点，数据节点上相关复制日志生成
 *    3.指定默认值的情况下，内部模式中也添加该字段的信息
 **************************************************************************************************/
main(test);

function test() {
  var clname = "schema_7";
  var schemaName = clname + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, DEFAULT_SCHEMA_FIELDS_DEFINE);
  var cl = commCreateCL(db, COMMCSNAME, clname, { EnableInfoSchema: true });
  cl.addSchema(schemaName);

  // 不指定默认值
  var columnName = "new_column";
  var columnDef = { Type: "string" };
  schema.addColumn(columnName, columnDef);
  checkColumnDef(db, schemaName, columnName, columnDef);

  // 指定默认值
  var columnName = "new_column2";
  var columnDef = { Type: "int32", ReadDefault: 10, WriteDefault: 5 };
  schema.addColumn(columnName, columnDef);
  checkColumnDef(db, schemaName, columnName, columnDef);

  commDropCL(db, COMMCSNAME, clname);
}
