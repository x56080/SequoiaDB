/***************************************************************************************************
 * @Description: 外部模式管理：修改与集合绑定的外部模式——删除字段场景
 * @ATCaseID: schema_8
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
 *    1.删除字段
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建集合绑定该外部模式
 *    3.删除外部模式的字段
 *    4.添加删除字段的同名字段
 * 期望结果：
 *    1.通过快照查看schema元数据中完成了对应修改
 *    2.ZHY TODO 影响数据节点，数据节点上相关复制日志生成
 *    3.ZHY TODO 内部模式中如果存在该字段，其被标记为删除状态
 **************************************************************************************************/
main(test);

function test() {
  var clname = "schema_8";
  var schemaName = clname + "_1";
  var columnName = "column1";
  var columns = {};
  columns[columnName] = { Type: "string" };
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, columns);
  var cl = db.getCS(COMMCSNAME).createCL(clname, { EnableInfoSchema: true });
  cl.addSchema(schemaName);
  schema.dropColumn(columnName);
  checkIfColumnNotExist(db, schemaName, columnName);
  var columnDef = { Type: "double", ReadDefault: 2.1, WriteDefault: 5.2 };
  schema.addColumn(columnName, columnDef );
  checkColumnDef(db, schemaName, columnName, columnDef);

  commDropCL(db, COMMCSNAME, clname);
}
