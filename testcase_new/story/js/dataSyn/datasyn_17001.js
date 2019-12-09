/************************************
*@Description: seqDB-17001 create multiple unique indexes ,do the following:
               1.insert datas 
               2.update _id
               3.insert datas
               4.check data synchronization               
*@author:      wuyan
*@date:        2018.12.28
**************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_IndexAndDataSyn_17001";
DataSyncTestCase.prototype.execTest = function()
{
   var expRecs = insertData( this.dbcl );

   this.dbcl.createIndex( "idxa", { 'inta': 1, 'str': 1, 'no': 1 }, true );
   this.dbcl.createIndex( "idxb", { 'fc': 1, 'no': -1 }, true );

   updateDatas( this.dbcl, expRecs.length );

   var sortCond = { 'inta': 1 };
   var expRecsAfterUpdate = getUpdateExpRecs( expRecs );
   checkDataContent( db, csName, clName, sortCond, expRecsAfterUpdate, "17001a", false );

   // insert 2W records again, with a range of 20000-40000,eg:no:[20000,39999]
   var beginNo = 20000;
   var expRecsAfterInsert = buckInsertData( this.dbcl, 20000, beginNo );
   var findCond = { 'no': { $gte: beginNo } };
   this.checkResult( sortCond, expRecsAfterInsert, "17001b", true, findCond );
}
main();

function updateDatas ( dbcl, nums )
{
   println( "---begin to update data." );
   for( var i = 0; i < nums; i++ )
   {
      dbcl.update( { $set: { '_id': i } }, { 'inta': i } );
   }
}

function getUpdateExpRecs ( expRecs )
{
   for( var i = 0; i < expRecs.length; i++ )
   {
      var item = expRecs[i];
      item._id = i;
   }
   return expRecs;
}


