/************************************
*@Description: seqDB-16991 create multiple unique indexes ,than insert datas check data synchronization               
*@author:      wuyan
*@date:        2018.12.27
**************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_IndexAndDataSyn_16991";
DataSyncTestCase.prototype.execTest = function()
{
   this.dbcl.createIndex( "idxa", { 'inta': 1 }, true );
   this.dbcl.createIndex( "idxb", { 'str': 1 }, true );
   this.dbcl.createIndex( "idxc", { 'fc': 1 }, true );

   var expRecs = insertData( this.dbcl );

   var sortCond = { no: 1 };
   this.checkResult( sortCond, expRecs, "16991" );
}
main();

