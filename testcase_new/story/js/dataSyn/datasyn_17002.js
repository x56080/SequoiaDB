/************************************
*@Description: seqDB-17002 create multiple unique indexes ,do the following:
               1.insert datas 
               2.update _id field
               3.update the non-_id field
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
   var clName = COMMCLNAME + "_IndexAndDataSyn_17002";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var groups = getOneGroups(db);   
   var groupName = groups[0].GroupName;
   var options = {ShardingKey:{no:1},Compressed:true, Group:groupName};
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, options);
   var insertNums = 20000;        
   var expRecs = buckInsertData( dbcl, insertNums);   
   
   dbcl.createIndex("idxa",{'inta':1,'str':1,'no':1},true);  
   dbcl.createIndex("idxb",{'fc':1,'no':-1},true);
   
   updateDatasOfId( dbcl, insertNums);  
      
   var sortCond = {'inta':1};
   println("---update the id field.");
   getUpdateExpRecs(expRecs, "_id");   
   checkDataContent(db,COMMCSNAME, clName, sortCond, expRecs, "17002a", false);
   checkInspectResult(COMMCSNAME, clName);    
  
   println("---update the non-_id field.");
   dbcl.update( { $set: { 'str': "testdatasyn17002" } } );   
   var expRecsAfterUpdate = getUpdateExpRecs(expRecs, "str");  
   checkDataContent(db,COMMCSNAME, clName, sortCond, expRecsAfterUpdate, "17002b", false);        
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


function getUpdateExpRecs(expRecs, field)
{
   for( var i = 0 ; i < expRecs.length; i++ )
   {
      var item = expRecs[i];
      if ( field == "_id")
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


