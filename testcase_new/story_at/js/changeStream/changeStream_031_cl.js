/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_031
 * @Author: Huang Youquan
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ========== =========================================================
 * 08/09/2023 Huang Youquan change stream
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    change stream on collection
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "changeStream_031";
var selectedGroup = selectGroupWithSecondary(db);
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 };

main(test);
function test() {
   var nodeCL = db.getCS(testConf.csName).getCL(testConf.clName);

   // start watch
   var cur = nodeCL.watch(StreamToken(), { MaxWaitTime: 0, Groups: [selectedGroup] });
   var document = { _id: 1, a: 1, b: 1 };
   var dbCS = db.getCS(testConf.csName);
   var dbCL = dbCS.getCL(testConf.clName);

   // insert
   dbCL.insert(document);
   checkChangeStreamInsertResult(cur, testConf.csName, testConf.clName, document);

   // update
   var documentKey = { _id: 1 };
   var updateAction = { $set: { a: 2 } };
   dbCL.update(updateAction, documentKey);
   checkChangeStreamUpdateResult(cur, testConf.csName, testConf.clName, documentKey, updateAction);

   // update multiple keys
   updateAction = { $set: { a: 3, b: 3 }, $inc: { c: 1 } };
   dbCL.update(updateAction, documentKey);
   checkChangeStreamUpdateResult(cur, testConf.csName, testConf.clName, documentKey, { $set: { a: 3, b: 3, c: 1 } });

   // delete
   dbCL.remove(documentKey);
   checkChangeStreamDeleteResult(cur, testConf.csName, testConf.clName, documentKey);

   // create index
   dbCL.createIndex('a', { a: 1 });
   checkChangeStreamResult(cur, testConf.csName, testConf.clName, "createix");

   // drop index
   dbCL.dropIndex('a');
   checkChangeStreamResult(cur, testConf.csName, testConf.clName, "deleteix");

   // alter collection
   dbCL.alter({ ShardingKey: { a: 1 }, AutoSplit: false });
   checkChangeStreamResult(cur, testConf.csName, testConf.clName, "invalidatecata");
   checkChangeStreamResult(cur, testConf.csName, testConf.clName, "alter");

   // insert
   dbCL.insert(document);
   checkChangeStreamInsertResult(cur, testConf.csName, testConf.clName, document);

   // truncate
   dbCL.truncate();
   checkChangeStreamResult(cur, testConf.csName, testConf.clName, "truncatecl");

   // drop
   dbCS.dropCL(testConf.clName);
   checkChangeStreamErrorResult(cur, true, SDB_DMS_NOTEXIST, "Collection [" + testConf.csName + "." + testConf.clName + "] has been dropped");

   cur.close();
}