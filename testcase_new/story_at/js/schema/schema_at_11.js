/***************************************************************************************************
 * @Description: 外部模式使用：给分区集合绑定外部模式，并增加字段、修改字段、删除字段
 * @ATCaseID: schema_11
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Bind external schema to a sharding collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.分区集合未开启内部模式的情况下给其绑定外部模式
 *    2.分区集合已开启内部模式的情况下给其绑定外部模式
 *    3.集合先绑定外部模式，并插入数据，再开启自动分区，校验数据是否正确
 * 测试步骤：
 *    1.创建外部模式
 *    2.创建分区集合
 *    3.给集合绑定外部模式
 *    4.增加字段、修改字段、删除字段
 * 期望结果：
 *    1.集合未开启内部模式的情况下报错
 *    2.集合的全部分区上的内部模式与外部模式均不冲突的情况下，绑定成功，InfoSchema元数据中记录集合名，集合元数据中记录InfoSchema的名字
 *    3.ZHY TODO存在冲突的情况下报错，内部模式无任何变更，数据节点无复制日志生成
 *    4.校验数据正确
 *    5.外部模式的字段变更正确
 *
 **************************************************************************************************/
main(test);

function test() {
  var disabledCLName = "disable_schema_11";
  var enabledCLName = "enable_schema_11";
  var schemaName = enabledCLName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, { column1: { Type: "string" } });
  var groupNames = commGetDataGroupNames(db);
  assert.equal(groupNames.length > 1, true);

  // 1.分区集合未开启内部模式的情况下给其绑定外部模式
  var disabledCL = db
    .getCS(COMMCSNAME)
    .createCL(disabledCLName, { ShardingKey: { id: 1 }, ShardingType: "hash", Group: groupNames[0] });
  disabledCL.split(groupNames[0], groupNames[1], { id: 2048 }, { id: 4096 });
  assert.tryThrow(SDB_OPERATION_INCOMPATIBLE, function () {
    disabledCL.addSchema(schemaName);
  });
  commDropCL(db, COMMCSNAME, disabledCLName);

  // 2.分区集合已开启内部模式的情况下给其绑定外部模式
  var cl = db.getCS(COMMCSNAME).createCL(enabledCLName, {
    EnableInfoSchema: true,
    ShardingKey: { id: 1 },
    ShardingType: "hash",
    Group: groupNames[0],
  });
  cl.split(groupNames[0], groupNames[1], { id: 2048 }, { id: 4096 });
  cl.addSchema(schemaName);
  checkIfCollectionBoundToSchema(db, COMMCSNAME, enabledCLName, schemaName);
  commDropCL(db, COMMCSNAME, enabledCLName);

  // 3.集合先绑定外部模式，并插入数据，再开启自动分区
  var cl = db.getCS(COMMCSNAME).createCL(enabledCLName, { EnableInfoSchema: true });
  var schemaName = enabledCLName + "_2";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    rid: { Type: "int32" },
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double", WriteDefault: 10.5 },
  });
  cl.addSchema(schemaName);

  var records = [];
  var expRecs = [];
  for (var i = 0; i < 10000; i++) {
    records.push({ rid: i });
    expRecs.push({ rid: i, b: 10.5 });
  }
  cl.insert(records);
  cl.enableSharding({ ShardingKey: { rid: 1 }, AutoSplit: true });
  var cursor = cl.find().sort({ rid: 1 });
  commCompareResults(cursor, expRecs);

  // 增加字段
  var columnName = "new_field";
  schema.addColumn(columnName, {
    Type: "int32",
    ReadDefault: 5,
    WriteDefault: 10,
  });
  checkColumnDef(db, schemaName, columnName, {
    Type: "int32",
    ReadDefault: 5,
    WriteDefault: 10,
  });

  // 修改字段
  schema.alterColumn(columnName, {
    WriteDefault: 20,
  });
  checkColumnDef(db, schemaName, columnName, {
    Type: "int32",
    ReadDefault: 5,
    WriteDefault: 20,
  });

  // 删除字段
  schema.dropColumn(columnName);
}
