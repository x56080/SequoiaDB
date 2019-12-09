/************************************
*@Description: seqDB-17002 create multiple unique indexes ,do the following:
               1.insert datas 
               2.update _id field
               3.update the non-_id field
               4.check data synchronization               
*@author:      wuyan
*@date:        2018.12.28
**************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_IndexAndDataSyn_17002";
DataSyncTestCase.prototype.execTest = function()
{
   var insertNums = 20000;
   var expRecs = buckInsertData( this.dbcl, insertNums );

   this.dbcl.createIndex( "idxa", { 'inta': 1, 'str': 1, 'no': 1 }, true );
   this.dbcl.createIndex( "idxb", { 'fc': 1, 'no': -1 }, true );

   updateDatasOfId( this.dbcl, insertNums );

   var sortCond = { 'inta': 1 };
   println( "---update the id field." );
   getUpdateExpRecs( expRecs, "_id" );
   this.checkResult( sortCond, expRecs, "17002a", false );

   println( "---update the non-_id field." );
   this.dbcl.update( { $set: { 'str': "testdatasyn17002" } } );
   var expRecsAfterUpdate = getUpdateExpRecs( expRecs, "str" );
   this.checkResult( sortCond, expRecs, "17002b", false );
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


function getUpdateExpRecs ( expRecs, field )
{
   for( var i = 0; i < expRecs.length; i++ )
   {
      var item = expRecs[i];
      if( field == "_id" )
      {
         item._id = i;
      }
      else
      {
         item.str = "testdatasyn17002";
      }
   }
   return expRecs;
}


