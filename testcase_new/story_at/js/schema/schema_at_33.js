/***************************************************************************************************
 * @Description: rename子表，校验schema编目信息记录的集合名仍为主表名
 * @ATCaseID: schema_33
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 03/02/2023 Zhou Hongye Rename sub-collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    rename子表，校验schema编目信息记录的集合名仍为主表名
 *
 * 测试步骤：
 *    1.创建主子表并挂载
 *    2.创建schema并绑定到主表上
 *    3.重命名子表
 *
 * 期望结果：
 *    编目信息中，schema仍与主表绑定
 *
 **************************************************************************************************/
main(test);

function test() {
  var mainCLName = "schema_33";
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

  var schemaName = mainCLName + "_1";
  commClearLegacySchema(db, schemaName);
  db.createSchema(schemaName, {
    a: { Type: "int32", ReadDefault: 5 },
    b: { Type: "double", WriteDefault: 10.5 },
  });

  maincl.addSchema(schemaName);
  db.getCS(COMMCSNAME).renameCL(subclName1, "new_sub");
  checkIfCollectionBoundToSchema(db,COMMCSNAME, mainCLName, schemaName);
}
