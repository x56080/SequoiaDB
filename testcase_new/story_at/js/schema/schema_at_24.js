/***************************************************************************************************
 * @Description: 历史数据升级
 * @ATCaseID: schema_24
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 02/17/2023 Zhou Hongye Historical data upgrade
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    已有历史数据的集合，开启内部模式并绑定外部模式，插入、更新数据，检查内部模式的字段及其默认值和记录数据是
 * 否符合预期
 *
 * 测试步骤：
 *    1.创建集合不开启其内部模式，向其中插入数据，模拟历史数据
 *    2.开启集合的内部模式，检查记录数据应未变化
 *    3.创建外部模式，并绑定到集合上，检查内部模式的字段及其默认值和记录数据是否符合预期
 *    4.变更外部模式的字段，检查内部模式的字段及其默认值和记录数据是否符合预期
 *    5.插入和更新部分数据，检查内部模式的字段及其默认值和记录数据是否符合预期
 *
 * 期望结果：
 *    所有校验均符合预期
 *
 **************************************************************************************************/
main(test);

function test() {
  var clName = "schema_24";
  var cl = commCreateCL(db, COMMCSNAME, clName, { EnableInfoSchema: false });

  var expRecs = [];
  var rid = 0;
  var record = { _id: rid++, a: 1 };
  cl.insert(record);
  expRecs.push(record);
  record = { _id: rid++, b: 3.5 };
  cl.insert(record);
  expRecs.push(record);
  record = { _id: rid++, c: "history" };
  cl.insert(record);
  expRecs.push(record);

  cl.alter({ EnableInfoSchema: true });
  // 开启内部模式不会触发内部模式进化，更新记录来触发
  cl.update({ $set: { b: 4.5 } }, { b: { $et: 3.5 } });
  expRecs[1].b = 4.5;
  checkInternalSchema(db, COMMCSNAME, clName, { _id: {}, b: {} });
  var cursor = cl.find().sort({ _id: 1 });
  commCompareResults(cursor, expRecs, false);

  var schemaName = clName + "_1";
  commClearLegacySchema(db, schemaName);
  var schema = db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5 }, // 设定字段a读默认值
    b: { Type: "double", WriteDefault: 10.5 }, // 设定字段b写默认值
    c: { Type: "double", ReadDefault: 11.5 }, // 与历史数据中同名字段的类型不同
  });
  cl.addSchema(schemaName);
  expRecs[0].c = 11.5;
  expRecs[1].a = 5;
  expRecs[1].c = 11.5;
  expRecs[2].a = 5;
  checkInternalSchema(db, COMMCSNAME, clName, {
    _id: {},
    a: { ReadDefault: 5 },
    b: { WriteDefault: 10.5 },
    c: { ReadDefault: 11.5 },
  });
  var cursor = cl.find().sort({ _id: 1 });
  commCompareResults(cursor, expRecs, false);

  // 变更字段
  schema.alterColumn("a", { WriteDefault: 10 });
  var record = { _id: rid++ };
  cl.insert(record);
  var expRecord = record;
  expRecord.a = 10;
  expRecord.b = 10.5;
  expRecs.push(expRecord);

  // 增加字段d
  schema.addColumn("d", { Type: "string", WriteDefault: "default d" });
  checkInternalSchema(db, COMMCSNAME, clName, {
    _id: {},
    a: { ReadDefault: 5, WriteDefault: 10 },
    b: { WriteDefault: 10.5 },
    c: { ReadDefault: 11.5 },
    d: { WriteDefault: "default d" },
  });

  // 删除字段d默认值
  schema.dropColumnDefault("d");
  checkInternalSchema(db, COMMCSNAME, clName, {
    _id: {},
    a: { ReadDefault: 5, WriteDefault: 10 },
    b: { WriteDefault: 10.5 },
    c: { ReadDefault: 11.5 },
    d: {},
  });

  // 删除字段d
  schema.dropColumn("d");
  checkInternalSchemaHasNoColumn(
    db,
    COMMCSNAME,
    clName,
    {
      _id: {},
      a: { ReadDefault: 5, WriteDefault: 10 },
      b: { WriteDefault: 10.5 },
      c: { ReadDefault: 11.5 },
      d: {},
    },
    "d"
  );

  var cursor = cl.find().sort({ _id: 1 });
  commCompareResults(cursor, expRecs, false);

  for (var i = 0; i < 3; ++i) {
    var id = rid++;
    cl.insert({ _id: id, a: 10 + i, b: 10.5 + i, e: "e" + i });
    expRecs.push({ _id: id, a: 10 + i, b: 10.5 + i, e: "e" + i });
  }

  checkInternalSchema(db, COMMCSNAME, clName, {
    _id: {},
    a: { ReadDefault: 5, WriteDefault: 10 },
    b: { WriteDefault: 10.5 },
    c: { ReadDefault: 11.5 },
    e: {},
  });
  var cursor = cl.find().sort({ _id: 1 });
  commCompareResults(cursor, expRecs, false);
}
