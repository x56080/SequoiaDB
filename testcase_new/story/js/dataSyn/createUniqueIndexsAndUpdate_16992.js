/************************************
*@Description: seqDB-16992 create multiple unique indexes and insert dates ,than update datas check data synchronization               
*@author:      wuyan
*@date:        2018.12.27
**************************************/
main();
function main()
{ 
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }
   var clName = COMMCLNAME + "_IndexAndDataSyn_16992";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var groups = getOneGroups(db);   
   var groupName = groups[0].GroupName;
   var options = {Compressed:true, Group:groupName};
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, options);        
   var expRecs = insertData( dbcl );   
   
   dbcl.createIndex("idxa",{'inta':1,'str':1},true);  
   dbcl.createIndex("idxb",{'fc':1},true);
   
   updateDatas( dbcl, expRecs);   
   
   var sortCond = {'inta':1};
   var expRecsAfterUpdate = getUpdateExpRecs(expRecs);
   checkDataContent(db, COMMCSNAME, clName, sortCond, expRecs, "16992");
   checkInspectResult(COMMCSNAME, clName);
   commDropCL(db, COMMCSNAME, clName, true, true);
}

function updateDatas( dbcl, expRecs)
{   
   println("---begin to update data.");
   dbcl.update({ $set: { 'str': "test16992" } } );
   for( var i = 0 ; i < expRecs.length; i++ )
   {
      dbcl.update({ $inc: { 'no': 200000 } }, { 'inta': i});
   }
}

function getUpdateExpRecs(expRecs)
{
   for( var i = 0 ; i < expRecs.length; i++ )
   {
      var item = expRecs[i];
      item.str = "test16992";
      item.no = i + 200000;
   }   
   return expRecs;
}

