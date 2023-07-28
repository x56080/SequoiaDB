import("../lib/basic_operation/commlib.js");
import("../lib/main.js");

// check the consistency of the record between the primary and the secondary
function checkRecordConsistency(cl) {
   var record = cl.find();
   //get actual result
   var expResult = [];
   while (record.next()) {
      expResult.push(record.current().toObj());
   }
   db.setSessionAttr({ PreferredInstance: "S" });
   var recordS = cl.find().sort({ _id: 1 });
   commCompareResults(recordS, expResult, false);
}

// check whether the transaction information is residual
function checkTransactionResidual() {
   var cursor = db.snapshot(SDB_SNAP_SYSTEM, { RawData: true }, { "TransInfo.TotalCount": 1 });
   while (cursor.next()) {
      assert.equal(cursor.current().toObj().TransInfo.TotalCount, 0);
   }
}