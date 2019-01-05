/************************************
*@Description: seqDB-17001 create multiple unique indexes ,do the following:
               1.insert datas 
               2.update _id
               3.insert datas
               4.check data synchronization               
*@author:      wuyan
*@date:        2018.12.28
**************************************/
main();
function main()
{ 
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }
   var clName = COMMCLNAME + "_IndexAndDataSyn_17001";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var groups = getOneGroups(db);   
   var groupName = groups[0].GroupName;
   var options = {ShardingKey:{no:1},Compressed:true, Group:groupName};
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, options);        
   var expRecs = insertData( dbcl );   
   
   dbcl.createIndex("idxa",{'inta':1,'str':1,'no':1},true);  
   dbcl.createIndex("idxb",{'fc':1,'no':-1},true);
   
   updateDatas( dbcl, expRecs.length);   
   
   var sortCond = {'inta':1};
   var expRecsAfterUpdate = getUpdateExpRecs(expRecs); 
   checkDataContent(db, COMMCSNAME, clName, sortCond, expRecsAfterUpdate, "17001a", false);     
  
   // insert 2W records again, with a range of 20000-40000,eg:no:[20000,39999]
   var beginNo = 20000;
   var expRecsAfterInsert = buckInsertData( dbcl, 20000, beginNo);
   var findCond = {'no':{$gte: beginNo}};   
   checkDataContent(db, COMMCSNAME, clName, sortCond, expRecsAfterInsert, "17001b", true, findCond);        
   checkInspectResult(COMMCSNAME, clName);   
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}

function updateDatas( dbcl, nums)
{   
   println("---begin to update data.");
   for( var i = 0 ; i < nums; i++ )
   {
      dbcl.update({ $set: { '_id': i } }, { 'inta': i});
   }
}

function getUpdateExpRecs(expRecs)
{
   for( var i = 0 ; i < expRecs.length; i++ )
   {
      var item = expRecs[i];
      item._id = i;     
   }   
   return expRecs;
}


