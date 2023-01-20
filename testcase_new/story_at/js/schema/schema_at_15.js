/***************************************************************************************************
 * @Description: 外部模式使用：删除集合时，如果有集合上绑定了外部模式，将自动将其同步删除
 * @ATCaseID: schema_15
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Automatically drop external schema when collection is deleted 
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.删除集合时，如果有集合上绑定了外部模式，将自动将其同步删除
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建普通集合，并绑定外部模式
 *    3.删除集合
 * 期望结果：
 *    删除成功
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_15";
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, DEFAULT_SCHEMA_FIELDS_DEFINE);
  var cl = commCreateCL(db, COMMCSNAME, clName, {EnableInfoSchema:true});
  cl.addSchema(schemaName);
  checkIfCollectionBoundToSchema(db, COMMCSNAME, clName, schemaName);
  commDropCL(db,COMMCSNAME,clName);
  assert.equal(db.list(SDB_LIST_SCHEMAS, {Name:schemaName}).size(), 0);
}