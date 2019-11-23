/***************************************************************************
@Description : seqDB-15977:更新记录，自增字段值保留，分区键与自增字段相同
@Modify list :
              2018-10-17  zhaoyu  Create
****************************************************************************/
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
	  return;
   }
   
   var clName = COMMCLNAME + "_15977_1";   
   commDropCL(db, COMMCSNAME, clName, true, true);
  
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, {ShardingKey:{id:1}});
   dbcl.insert({a:1});
   
   dbcl.createAutoIncrement({Field:"id"});
   dbcl.insert({a:2});
   dbcl.update({$replace:{c:100}});
   var actR = dbcl.find().sort({_id:1});
   var expR = [{c:100},{c:100,id:1}];
   checkRec(actR, expR);
   println("---check replace other field success");
   
   dbcl.update({$set:{c:1000}});
   var actR = dbcl.find().sort({_id:1});
   var expR = [{c:1000},{c:1000,id:1}];
   checkRec(actR, expR);
   println("---check set other field success");
   
   dbcl.update({$set:{id:1000}});
   var actR = dbcl.find().sort({_id:1});
   var expR = [{c:1000},{c:1000,id:1}];
   checkRec(actR, expR);
   println("---check set autoIncrement field success");
   
   dbcl.insert({a:1});
   var actR = dbcl.find().sort({_id:1});
   var expR = [{c:1000},{c:1000,id:1},{a:1,id:2}];
   checkRec(actR, expR);
   println("---check insert after set autoIncrement field success");
   
   dbcl.update({$unset:{id:""}}, {id:{$exists:1}});
   var actR = dbcl.find().sort({_id:1});
   var expR = [{c:1000},{c:1000,id:1},{a:1,id:2}];
   checkRec(actR, expR);
   println("---check unset autoIncrement field success");
   
   dbcl.update({$replace:{id:100}});
   var actR = dbcl.find().sort({_id:1});
   var expR = [{},{id:1},{id:2}];
   checkRec(actR, expR);
   println("---check replace autoIncrement field success");
  
   commDropCL(db, COMMCSNAME, clName, true, true);
}
try
{
   main();
}
catch(e)
{
   if ( e.constructor === Error )
   {
      println(e.stack) ;  
   }
   throw e ;
}
