/***************************************************************************************************
 * @Description: 外部模式管理：修改与集合绑定的外部模式——修改字段场景
 * @ATCaseID: schema_9
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
 *    1.修改与集合绑定的外部模式的字段
 * 测试步骤：
 *    1.创建外部模式
 *    2.修改外部模式的字段
 * 期望结果：
 *    1.无冲突的情况下，修改正常完成，通过infoSchema快照查看schema元数据，相关信息已更新
 *    2.存在冲突的情况下对外报错，内部模式无任何变更，ZHY TODO 数据节点无日志生成
 *
 **************************************************************************************************/
main(test);

function test() {
  var clname = "schema_9";
  var schemaName = clname + "_1";
  var columnName = "column1";
  var columns = {};
  columns[columnName] = { Type: "double" };
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, columns);
  var cl = commCreateCL(db, COMMCSNAME, clname, { EnableInfoSchema: true });
  cl.addSchema(schemaName);
  var modifiedColumnDef = { Type: "double", WriteDefault: 2.1 };
  schema.alterColumn(columnName, modifiedColumnDef);
  checkColumnDef(db, schemaName, columnName, modifiedColumnDef);

  commDropCL(db, COMMCSNAME, clname);
}
