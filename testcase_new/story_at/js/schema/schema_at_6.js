/***************************************************************************************************
 * @Description: 外部模式管理：修改未与集合绑定的外部模式
 * @ATCaseID: schema_6
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Modify the external schema that is not bound to a collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.增加字段
 *    2.修改字段
 *    3.删除字段
 * 测试步骤：
 *    1.创建外部模式
 *    2.修改外部模式
 * 期望结果：
 *    1.通过快照查看schema元数据中完成了对应修改
 *    2.ZHY TODO 不影响数据节点，数据节点上无相关复制日志生成
 **************************************************************************************************/
main(test);

function test() {
  // 1.增加字段
  var schemaName = "schema_6_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, DEFAULT_SCHEMA_FIELDS_DEFINE);
  var columnName = "new_column";
  var columnDef = { Type: "int32", ReadDefault: 10, WriteDefault: 5 };
  schema.addColumn(columnName, columnDef);
  checkColumnDef(db, schemaName, columnName, columnDef);

  // 2.修改字段
  var modifiedColumnDef = { Type: "double", ReadDefault: 5.2, WriteDefault: 2.1 };
  schema.alterColumn(columnName, modifiedColumnDef);
  checkColumnDef(db, schemaName, columnName, modifiedColumnDef);

  // 3.删除字段
  schema.dropColumn(columnName);
  checkIfColumnNotExist(db, schemaName, columnName);

  db.dropSchema(schemaName);
}
