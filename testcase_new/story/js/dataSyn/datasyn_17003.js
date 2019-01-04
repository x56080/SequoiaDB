/************************************
*@Description: seqDB-17003 create multiple unique indexes ,do the following:
               1.insert datas 
               2.update _id field
               3.delete datas
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
   var clName = COMMCLNAME + "_IndexAndDataSyn_17003";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var groups = getOneGroups(db);   
   var groupName = groups[0].GroupName;
   var options = {ShardingKey:{no:1},Compressed:true, Group:groupName};
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, options);
   var insertNums = 30000;        
   var expRecs = buckInsertData( dbcl, insertNums);   
   
   dbcl.createIndex("idxa",{'inta':1,'str':1,'no':1},true);  
   dbcl.createIndex("idxb",{'fc':1,'no':-1},true);
   
   var updateNums = 20000;
   updateDatasOfId( dbcl, updateNums);  
   getUpdateExpRecs(expRecs, updateNums);
      
   println("---begin to delete datas.");
   var deleteSerial = 10000;
   dbcl.remove( { no:{'$gte':deleteSerial}});
   expRecs.splice(deleteSerial);
   
   var sortCond = {'inta':1};   
   checkDataContent(db, COMMCSNAME, clName, sortCond, expRecs, "17003");        
   checkInspectResult(COMMCSNAME, clName);     
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}

function updateDatasOfId( dbcl, nums)
{   
   println("---begin to update datas.");
   for( var i = 0 ; i < nums; i++ )
   {
      dbcl.update({ $set: { '_id': i } }, { 'inta': i});
   }
}


function getUpdateExpRecs(expRecs, updateNums)
{
   for( var i = 0 ; i < updateNums; i++ )
   {
      var item = expRecs[i];      
      item._id = i;         
   }   
   return expRecs;
}


