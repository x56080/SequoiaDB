/************************************
*@Description: seqDB-16991 create multiple unique indexes ,than insert datas check data synchronization               
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
   var clName = COMMCLNAME + "_IndexAndDataSyn_16991";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var groups = getOneGroups(db);   
   var groupName = groups[0].GroupName;
   var options = {Compressed:true, Group:groupName};
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, options);        

   dbcl.createIndex("idxa",{'inta':1},true);
   dbcl.createIndex("idxb",{'str':1},true);
   dbcl.createIndex("idxc",{'fc':1},true);
   
   var expRecs = insertData( dbcl );
   var sortCond = {no:1};
   checkDataContent(db, COMMCSNAME, clName, sortCond, expRecs, "16991");        
   checkResult(COMMCSNAME, clName,  groups, sortCond, expRecs, false);  
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}


