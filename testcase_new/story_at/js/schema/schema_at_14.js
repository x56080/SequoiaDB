/***************************************************************************************************
 * @Description: 外部模式使用：插入数据后，进行外部模式变更，校验数据
 * @ATCaseID: schema_14
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/09/2023 Zhou Hongye Modify schema and validate records
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    1.集合中已有记录，为其绑定外部模式，所有记录中均可正常读出设置了ReadDefault的值
 *    2.向绑定了外部模式的集合中写入记录，设定了WriteDefault的字段如果在记录中未被赋值，将会被赋予默认值
 *    3.变更绑定到集合上的外部模式，增加字段
 *    4.变更绑定到集合上的外部模式，删除字段
 *    5.修改绑定到集合上的外部模式的字段名
 *    6.修改绑定到集合上的外部模式的默认值
 * 测试步骤：
 *    1.创建集合，创建外部模式
 *    2.根据场景变更外部模式
 *    3.检查数据
 * 期望结果：
 *    每个场景的数据结果均符合预期
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_14_1";
  // 普通集合
  var cl = commCreateCL(db, COMMCSNAME, clName, { EnableInfoSchema: true });
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double", WriteDefault: 10.5 },
  });
  testColumnChanges(cl, schemaName);
  commDropCL(db, COMMCSNAME, clName);

  // 数据库分区的集合
  var clName = "schema_14_2";
  var cl = db.getCS(COMMCSNAME).createCL(clName, {
    EnableInfoSchema: true,
    ShardingKey: { id: 1 },
    ShardingType: "hash",
    Group: "db1",
  });
  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double", WriteDefault: 10.5 },
  });
  cl.split("db1", "db2", { id: 2048 }, { id: 4096 });
  testColumnChanges(cl, schemaName);
  commDropCL(db, COMMCSNAME, clName);

  // 表分区的集合
  var mainCLName = "schema_14_3";
  var maincl = commCreateCL(db, COMMCSNAME, mainCLName, {
    IsMainCL: true,
    ShardingKey: { rid: 1 },
    ShardingType: "range",
    EnableInfoSchema: true,
  });
  var subclName1 = mainCLName + "_sub1";
  var subclName2 = mainCLName + "_sub2";
  commCreateCL(db, COMMCSNAME, subclName1, { EnableInfoSchema: true });
  commCreateCL(db, COMMCSNAME, subclName2, { EnableInfoSchema: true });

  maincl.attachCL(COMMCSNAME + "." + subclName1, {
    LowBound: { rid: 0 },
    UpBound: { rid: 5 },
  });
  maincl.attachCL(COMMCSNAME + "." + subclName2, {
    LowBound: { rid: 5 },
    UpBound: { rid: 10 },
  });

  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double", WriteDefault: 10.5 },
  });
  testColumnChanges(maincl, schemaName);
  commDropCL(db, COMMCSNAME, mainCLName);
}

function testColumnChanges(cl, schemaName) {
  // 1.集合中已有记录，为其绑定外部模式，所有记录中均可正常读出设置了ReadDefault的字段值
  var records = [];
  var expRecs = [];
  var i = 0;
  for (; i < 5; i++) {
    records.push({ rid: i });
    expRecs.push({ rid: i, a: 5 });
  }
  for (; i < 10; i++) {
    records.push({ rid: i, a: i });
    expRecs.push({ rid: i, a: i });
  }
  cl.insert(records);
  cl.addSchema(schemaName);
  var schema = db.getSchema(schemaName);
  var cursor = cl.find().sort({rid:1});
  commCompareResults(cursor, expRecs);

  // 2.向绑定了外部模式的集合中写入记录，设定了WriteDefault的字段如果在记录中未被赋值，将会被赋予默认值
  cl.truncate();
  var records = [];
  var expRecs = [];
  for (var i = 0; i < 5; i++) {
    records.push({ rid: i });
    expRecs.push({ rid: i, a: 5, b: 10.5 });
  }
  cl.insert(records);
  var cursor = cl.find().sort({rid:1});
  commCompareResults(cursor, expRecs);

  // 3.变更绑定到集合上的外部模式，增加字段
  schema.addColumn("c", { Type: "string", ReadDefault: "read default" });
  var expRecs = [];
  for (var i = 0; i < 5; i++) {
    expRecs.push({ rid: i, a: 5, b: 10.5, c: "read default" });
  }
  var cursor = cl.find().sort({rid:1});
  commCompareResults(cursor, expRecs);

  // 4.变更绑定到集合上的外部模式，删除字段
  schema.dropColumn("a");
  schema.dropColumn("b");
  var expRecs = [];
  for (var i = 0; i < 5; i++) {
    expRecs.push({ rid: i, c: "read default" });
  }
  var cursor = cl.find().sort({rid:1});
  commCompareResults(cursor, expRecs);

  // 5.修改绑定到集合上的外部模式的字段名
  schema.renameColumn("c", "new_c");
  var expRecs = [];
  for (var i = 0; i < 5; i++) {
    expRecs.push({ rid: i, new_c: "read default" });
  }
  var cursor = cl.find().sort({rid:1});
  commCompareResults(cursor, expRecs);

  // 6.修改绑定到集合上的外部模式的默认值
  schema.dropColumn("new_c");
  schema.addColumn("d", { Type: "decimal", WriteDefault: { $decimal: "0.3333" } });
  cl.truncate();
  var expRecs = [];
  var i = 0;
  for (; i < 5; i++) {
    cl.insert({ rid: i });
    expRecs.push({ rid: i, d: { $decimal: "0.3333" } });
  }
  schema.alterColumn("d", { Type: "decimal", WriteDefault: { $decimal: "1.6666" } });
  for (; i < 10; i++) {
    cl.insert({ rid: i });
    expRecs.push({ rid: i, d: { $decimal: "1.6666" } });
  }

  var cursor = cl.find().sort({rid:1});
  commCompareResults(cursor, expRecs);
}
