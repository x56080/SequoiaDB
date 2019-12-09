/************************************
*@Description: seqDB-17003 create multiple unique indexes ,do the following:
               1.insert datas 
               2.update _id field
               3.delete datas
               4.check data synchronization               
*@author:      wuyan
*@date:        2018.12.28
**************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_IndexAndDataSyn_17003";
DataSyncTestCase.prototype.execTest = function()
{
   var insertNums = 30000;
   var expRecs = buckInsertData( this.dbcl, insertNums );

   this.dbcl.createIndex( "idxa", { 'inta': 1, 'str': 1, 'no': 1 }, true );
   this.dbcl.createIndex( "idxb", { 'fc': 1, 'no': -1 }, true );

   var updateNums = 20000;
   updateDatasOfId( this.dbcl, updateNums );
   getUpdateExpRecs( expRecs, updateNums );

   println( "---begin to delete datas." );
   var deleteSerial = 10000;
   this.dbcl.remove( { no: { '$gte': deleteSerial } } );
   expRecs.splice( deleteSerial );

   var sortCond = { 'inta': 1 };
   this.checkResult( sortCond, expRecs, "17003" );
}
main();
function updateDatasOfId ( dbcl, nums )
{
   println( "---begin to update datas." );
   for( var i = 0; i < nums; i++ )
   {
      dbcl.update( { $set: { '_id': i } }, { 'inta': i } );
   }
}

function getUpdateExpRecs ( expRecs, updateNums )
{
   for( var i = 0; i < updateNums; i++ )
   {
      var item = expRecs[i];
      item._id = i;
   }
   return expRecs;
}


