/************************************
*@Description: seqDB-16993 create multiple unique indexes and insert datas,than delete datas check data synchronization               
*@author:      wuyan
*@date:        2018.12.27
**************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_IndexAndDataSyn_16993";
DataSyncTestCase.prototype.execTest = function()
{
   this.dbcl.createIndex( "idxa", { 'inta': 1 }, true );
   this.dbcl.createIndex( "idxb", { 'str': 1 }, true );
   this.dbcl.createIndex( "idxc", { 'fc': 1 }, true );

   var expRecs = insertData( this.dbcl );

   println( "---begin to remove." );
   this.dbcl.remove( { no: { "$gt": 0 } } );

   var expRecsAfterRemove = [];
   expRecsAfterRemove.push( expRecs[0] );
   var sortCond = { no: 1 };
   this.checkResult( sortCond, expRecsAfterRemove, "16993" );
}
main();




