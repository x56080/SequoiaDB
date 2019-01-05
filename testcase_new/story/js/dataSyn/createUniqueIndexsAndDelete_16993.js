/************************************
*@Description: seqDB-16993 create multiple unique indexes and insert datas,than delete datas check data synchronization               
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
   var clName = COMMCLNAME + "_IndexAndDataSyn_16993";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var groups = getOneGroups(db);   
   var groupName = groups[0].GroupName;
   var options = {Compressed:true, Group:groupName};
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, options);        

   dbcl.createIndex("idxa",{'inta':1},true);
   dbcl.createIndex("idxb",{'str':1},true);
   dbcl.createIndex("idxc",{'fc':1},true);
   
   var expRecs = insertData( dbcl );
   
   println("---begin to remove.");
   dbcl.remove({ no : {"$gt":0}});
   var expRecsAfterRemove = [];
   expRecsAfterRemove.push(expRecs[0]);
   var sortCond = {no:1};
   checkDataContent(db, COMMCSNAME, clName, sortCond, expRecsAfterRemove, "16993");        
   checkInspectResult(COMMCSNAME, clName); 
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}


