/***************************************************************************************************
 * @Description: 验证集合快照 TotalOverflowRecords 和 TotalOverflowRead 的正确性
 * @ATCaseID: overflow_at_1
 * @Author: FangJiabin
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                   并在 Testlink 系统中标记本用例文件名）
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 07/24/2022 FangJiabin  Test the correctness of TotalOverflowRecords and TotalOverflowRead in snapshot cl
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    验证集合快照 TotalOverflowRecords 和 TotalOverflowRead 字段的正确性
 *    1. 普通表（分区表和子表这两个字段的计算跟普通表一样，所以用普通表覆盖测试即可）；
 *    2. 主子表（查看主表的集合信息）；
 * 测试步骤：
 *    测试准备：修改配置参数 overflowratio = 0，方便生成 overflow 记录，修改之前先保存旧参数用于还原环境；
 *
 *    非事务场景：
 *       普通表场景：
 *          1. 创建普通表，插入测试数据，查看集合快照；
 *          2. 同一条记录，更新多次，查看集合快照；
 *          3. 更新跟步骤 2 不相同的记录，查看集合快照；
 *          4. 删除 overflow 记录，查看集合快照；
 *          5. 删除普通记录，查看集合快照；
 *          6. 执行 resetSnapshot，查看集合快照；
 *          7. 更新 overflow 记录，查看集合快照，再 truncate 集合，查看集合快照；
 *
 *       主子表场景(集合快照汇总主表信息功能目前已被屏蔽，不过 getDetail() 接口返回的结果集跟集合快照是一样的，所以可以通过 getDetail() 来验证主子表场景)：
 *          1. 创建主子表（一个主表，两个子表，都在同一个数据组），插入测试数据，查看集合快照；
 *          2. 主表更新记录，子表 1 和子表 2 的同一条记录，更新多次，通过 getDetail() 查看集合信息；
 *          3. 主表更新跟步骤 2 不相同的记录，通过 getDetail() 查看集合信息；
 *          4. 删除 overflow 记录，通过 getDetail() 查看集合信息；
 *          5. 删除普通记录，通过 getDetail() 查看集合信息；
 *          6. 执行 resetSnapshot，通过 getDetail() 查看集合信息；
 *          7. 更新 overflow 记录，通过 getDetail() 查看集合信息，再 truncate 主表，通过 getDetail() 查看集合信息；
 *
 *    事务场景（测试事务回滚场景，正常事务提交，上面场景已经能覆盖）：
 *       1. 创建普通表，插入测试数据；
 *       2. 开启事务，更新记录，查看集合快照，然后回滚事务，查看集合快照；
 *       3. 更新记录；
 *       4. 开启事务，删除记录，查看集合快照，然后回滚事务，查看集合快照；
 *
 *    清理环境：
 *       1. 删除集合空间；
 *       2. 复原配置参数；
 *
 * 期望结果：
 *    每个步骤都能成功执行，关键步骤执行结果说明如下：
 *
 *    非事务场景：
 *       普通表：
 *       第 1 步，预期结果是 TotalOverflowRecords 和 TotalOverflowRead 字段都为 0；
 *       第 2 步，预期结果是：
 *          (1) 第一次更新，TotalOverflowRecords = 1，TotalOverflowRead = 0；
 *          (2) 第二次更新，TotalOverflowRecords = 1，TotalOverflowRead = 2；
 *          (3) 第三次更新，TotalOverflowRecords = 1，TotalOverflowRead = 4；
 *          ( update记录，会先查找记录，如果该记录是 overflow 记录，会跳转去找该记录的数据，判断该记录是否符合条件，TotalOverflowRead 会加 1，
 *            然后再更新该 overflow 记录，更新时候会跳转去找该记录的数据做更新操作，TotalOverflowRead 会加 1 )
 *       第 3 步，预期结果是 TotalOverflowRecords = 2，TotalOverflowRead = 4；
 *       第 4 步，预期结果是 TotalOverflowRecords = 1，TotalOverflowRead = 5；
 *       第 5 步，预期结果是 TotalOverflowRecords = 1，TotalOverflowRead = 5；
 *       第 6 步，预期结果是 TotalOverflowRecords = 1，TotalOverflowRead = 0；
 *       第 7 步，预期结果是:
 *          (1) 更新 overflow 记录之后，TotalOverflowRecords = 1，TotalOverflowRead = 2；
 *          (2) truncate 之后，TotalOverflowRecords = 0，TotalOverflowRead = 2；
 *
 *       主子表：
 *       第 1 步，预期结果是 TotalOverflowRecords = 0，TotalOverflowRead = 0；
 *       第 2 步，预期结果是：
 *          (1) 第一次更新，TotalOverflowRecords = 2，TotalOverflowRead = 0；
 *          (2) 第二次更新，TotalOverflowRecords = 2，TotalOverflowRead = 4；
 *          (3) 第三次更新，TotalOverflowRecords = 2，TotalOverflowRead = 8；
 *       第 3 步，预期结果是 TotalOverflowRecords = 3，TotalOverflowRead = 8；
 *       第 4 步，预期结果是 TotalOverflowRecords = 2，TotalOverflowRead = 9；
 *       第 5 步，预期结果是 TotalOverflowRecords = 2，TotalOverflowRead = 9；
 *       第 6 步，预期结果是 TotalOverflowRecords = 2，TotalOverflowRead = 0；
 *       第 7 步，预期结果是：
 *          (1) 更新 overflow 记录之后，TotalOverflowRecords = 2，TotalOverflowRead = 2；
 *          (2) truncate 之后，TotalOverflowRecords = 0，TotalOverflowRead = 2；
 *
 *    事务场景：
 *    第 2 步，预期结果是：
 *       (1) 更新记录后，TotalOverflowRecords = 1，TotalOverflowRead = 0；
 *       (2) 回滚事务后，TotalOverflowRecords = 0，TotalOverflowRead = 2；
 *    第 3 步，预期结果是 TotalOverflowRecords = 1，TotalOverflowRead = 2；
 *    第 4 步，预期结果是：
 *       (1) 删除记录后，TotalOverflowRecords = 0，TotalOverflowRead = 3；
 *       (2) 回滚事务后，TotalOverflowRecords = 1，TotalOverflowRead = 4；
 **************************************************************************************************/

var oldConf;
var csName = "overflow_at_1_cs";

main(test);

function test() {
  setUp();

  testNormalCl();

  testMainAndSubCl();

  tearDown();
}

function setUp(cl) {
  commDropCS(db, csName, true);
  // before updating config, we should save old config firstly
  var ret = db.snapshot(
    SDB_SNAP_CONFIGS,
    {},
    { overflowratio: 1, transisolation: 1 }
  );
  oldConf = ret.current().toObj();
  db.updateConf({ overflowratio: 0, transisolation: 1 });
}

function tearDown() {
  // restore conf
  db.updateConf(oldConf);
  commDropCS(db, csName, true);
}

function testNormalCl() {
  var clName = "overflow_at_1_normal_cl";
  var clFullName = csName + "." + clName;
  var cl = commCreateCL(db, csName, clName, { ReplSize: 0 }, true, true);
  for (var i = 0; i < 10; i++) {
    cl.insert({ _id: i });
  }
  checkSnapOverflow(clFullName, 0, 0);

  // update the same record
  cl.update({ $set: { a: 1 } }, { _id: 0 });
  checkSnapOverflow(clFullName, 1, 0);

  cl.update({ $set: { b: 1 } }, { _id: 0 });
  checkSnapOverflow(clFullName, 1, 2);

  cl.update({ $set: { c: 1 } }, { _id: 0 });
  checkSnapOverflow(clFullName, 1, 4);

  // update the different record
  cl.update({ $set: { a: 1 } }, { _id: 1 });
  checkSnapOverflow(clFullName, 2, 4);

  // remove a overflow record
  cl.remove({ _id: 1 });
  checkSnapOverflow(clFullName, 1, 5);

  // remove a normal record
  cl.remove({ _id: 2 });
  checkSnapOverflow(clFullName, 1, 5);

  db.resetSnapshot();
  checkSnapOverflow(clFullName, 1, 0);

  cl.update({ $set: { d: 1 } }, { _id: 0 });
  checkSnapOverflow(clFullName, 1, 2);
  cl.truncate();
  checkSnapOverflow(clFullName, 0, 2);
  db.resetSnapshot();
  checkSnapOverflow(clFullName, 0, 0);

  testTrans();
}

function testTrans() {
  var clName = "overflow_at_1_normal_trans_cl";
  var groupName = commGetGroups(db)[0][0].GroupName;
  var clFullName = csName + "." + clName;
  var cl = commCreateCL(
    db,
    csName,
    clName,
    { ReplSize: 0, Group: groupName },
    true,
    true
  );

  cl.insert({ _id: 0 });

  db.transBegin();
  cl.update({ $set: { a: 1 } }, { _id: 0 });
  checkSnapOverflow(clFullName, 1, 0);
  db.transRollback();
  checkSnapOverflow(clFullName, 0, 2);

  cl.update({ $set: { a: 1 } }, { _id: 0 });
  checkSnapOverflow(clFullName, 1, 2);
  db.transBegin();
  cl.remove({ _id: 0 });
  checkSnapOverflow(clFullName, 0, 3);
  db.transRollback();
  checkSnapOverflow(clFullName, 1, 4);
}

function testMainAndSubCl() {
  var mainCLName = "overflow_at_1_main_cl";
  var subCLName1 = "overflow_at_1_sub_cl1";
  var subCLName2 = "overflow_at_1_sub_cl2";
  var groupName = commGetGroups(db)[0][0].GroupName;

  createMainAndSubCL(groupName, mainCLName, subCLName1, subCLName2);
  var cl = db.getCS(csName).getCL(mainCLName);
  checkDetailOverflow(cl, 0, 0);

  cl.update({ $set: { b: 1 } }, { _id: 0 });
  cl.update({ $set: { b: 1 } }, { _id: 3 });
  checkDetailOverflow(cl, 2, 0);

  cl.update({ $set: { c: 1 } }, { _id: 0 });
  cl.update({ $set: { c: 1 } }, { _id: 3 });
  checkDetailOverflow(cl, 2, 4);

  cl.update({ $set: { d: 1 } }, { _id: 0 });
  cl.update({ $set: { d: 1 } }, { _id: 3 });
  checkDetailOverflow(cl, 2, 8);

  cl.update({ $set: { b: 1 } }, { _id: 4 });
  checkDetailOverflow(cl, 3, 8);

  cl.remove({ _id: 0 });
  checkDetailOverflow(cl, 2, 9);

  cl.remove({ _id: 1 });
  checkDetailOverflow(cl, 2, 9);

  db.resetSnapshot();
  checkDetailOverflow(cl, 2, 0);

  cl.update({ $set: { e: 1 } }, { _id: 3 });
  checkDetailOverflow(cl, 2, 2);
  cl.truncate();
  checkDetailOverflow(cl, 0, 2);
}

function checkDetailOverflow(
  cl,
  expTotalOverflowRecords,
  expTotalOverflowRead
) {
  var cursor = cl.getDetail();
  while (cursor.next()) {
    var Details = cursor.current().toObj().Details;
    var info = Details[0];
    //println("===== info: " + JSON.stringify(info));

    if (info.TotalOverflowRecords != expTotalOverflowRecords) {
      throw new Error(
        "Invalid TotalOverflowRecords[" +
          info.TotalOverflowRecords +
          "] in cl detail. TotalOverflowRecords must be " +
          expTotalOverflowRecords
      );
    }

    if (info.TotalOverflowRead != expTotalOverflowRead) {
      throw new Error(
        "Invalid TotalOverflowRead[" +
          info.TotalOverflowRecords +
          "] in cl detail. TotalOverflowRead must be " +
          expTotalOverflowRead
      );
    }
  }
}

function checkSnapOverflow(
  clFullName,
  expTotalOverflowRecords,
  expTotalOverflowRead
) {
  var cursor = db.snapshot(
    SDB_SNAP_COLLECTIONS,
    { Name: clFullName, RawData: true },
    {
      "Details.TotalOverflowRecords": 1,
      "Details.TotalOverflowRead": 1,
    }
  );
  while (cursor.next()) {
    var Details = cursor.current().toObj().Details;
    var info = Details[0];
    //println("===== info: " + JSON.stringify(info));

    if (info.TotalOverflowRecords != expTotalOverflowRecords) {
      throw new Error(
        "Invalid TotalOverflowRecords[" +
          info.TotalOverflowRecords +
          "] in cl snapshot. TotalOverflowRecords must be " +
          expTotalOverflowRecords
      );
    }

    if (info.TotalOverflowRead != expTotalOverflowRead) {
      throw new Error(
        "Invalid TotalOverflowRead[" +
          info.TotalOverflowRecords +
          "] in cl snapshot. TotalOverflowRead must be " +
          expTotalOverflowRead
      );
    }
  }
}

function createMainAndSubCL(groupName, mainCLName, subCLName1, subCLName2) {
  var mainCL = commCreateCL(
    db,
    csName,
    mainCLName,
    {
      IsMainCL: true,
      ShardingKey: { a: 1 },
      ShardingType: "range",
      ReplSize: 0,
    },
    true,
    true
  );
  commCreateCL(
    db,
    csName,
    subCLName1,
    {
      ShardingKey: { a: 1 },
      ShardingType: "range",
      Group: groupName,
      ReplSize: 0,
    },
    true,
    true
  );
  commCreateCL(
    db,
    csName,
    subCLName2,
    {
      ShardingKey: { a: 1 },
      ShardingType: "range",
      Group: groupName,
      ReplSize: 0,
    },
    true,
    true
  );

  mainCL.attachCL(csName + "." + subCLName1, {
    LowBound: { a: 0 },
    UpBound: { a: 1000 },
  });
  mainCL.attachCL(csName + "." + subCLName2, {
    LowBound: { a: 1000 },
    UpBound: { a: 2000 },
  });

  for (var i = 0; i < 10; i++) {
    mainCL.insert({ _id: i, a: i });
  }

  for (var i = 1001; i < 1011; i++) {
    mainCL.insert({ _id: i, a: i });
  }
}
