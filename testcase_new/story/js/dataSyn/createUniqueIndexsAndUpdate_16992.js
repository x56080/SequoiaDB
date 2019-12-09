/************************************
*@Description: seqDB-16992 create multiple unique indexes and insert dates ,than update datas check data synchronization               
*@author:      wuyan
*@date:        2018.12.27
**************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_IndexAndDataSyn_16992";
DataSyncTestCase.prototype.execTest = function()
{
   var expRecs = insertData( this.dbcl );

   this.dbcl.createIndex( "idxa", { 'inta': 1, 'str': 1 }, true );
   this.dbcl.createIndex( "idxb", { 'fc': 1 }, true );

   updateDatas( this.dbcl, expRecs );

   var sortCond = { 'inta': 1 };
   var expRecsAfterUpdate = getUpdateExpRecs( expRecs );
   this.checkResult( sortCond, expRecsAfterUpdate, "16992" );
}
main();

function updateDatas ( dbcl, expRecs )
{
   println( "---begin to update data." );
   dbcl.update( { $set: { 'str': "test16992" } } );
   for( var i = 0; i < expRecs.length; i++ )
   {
      dbcl.update( { $inc: { 'no': 200000 } }, { 'inta': i } );
   }
}

function getUpdateExpRecs ( expRecs )
{
   for( var i = 0; i < expRecs.length; i++ )
   {
      var item = expRecs[i];
      item.str = "test16992";
      item.no = i + 200000;
   }
   return expRecs;
}

