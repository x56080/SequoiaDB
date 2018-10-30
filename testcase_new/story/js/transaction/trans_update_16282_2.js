/***************************************************************************
@Description : seqDB-16282:事务中执行replace shardingKey/自增字段，回滚功能验证,分区键与自增字段不同 
@Modify list :
              2018-10-30  zhaoyu  Create
****************************************************************************/
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
	  return;
   }
   
   var clName = COMMCLNAME + "_16282_1";   
   commDropCL(db, COMMCSNAME, clName, true, true);
  
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, {ShardingKey:{a:1}});
   dbcl.insert({a:1});
   
   //dbcl.createAutoIncrement({Field:"id"});
   dbcl.insert({a:2});
   dbcl.insert({b:2});
   
   db.transBegin();
   dbcl.update({$replace:{a:100}});
   db.transRollback();
   var actR = dbcl.find().sort({_id:1});
   var expR = [{a:1},{a:2},{b:2}];
   checkRec(actR, expR);
   println("---check replace shardingKey, rollback success");
   
   db.transBegin();
   dbcl.update({$replace:{id:200,a:200,c:200}});
   db.transRollback();
   var actR = dbcl.find().sort({_id:1});
   var expR = [{a:1},{a:2},{b:2}];
   checkRec(actR, expR);
   println("---check replace shardingKey and other field, rollback success");
   
   db.transBegin();
   dbcl.update({$replace:{a:100}});
   db.transCommit();
   var actR = dbcl.find().sort({_id:1});
   var expR = [{a:1},{a:2},{}];
   checkRec(actR, expR);
   println("---check replace shardingKey, commit success");
   
   db.transBegin();
   dbcl.update({$replace:{id:200,a:200,c:200}});
   db.transCommit();
   var actR = dbcl.find().sort({_id:1});
   var expR = [{id:200,a:1,c:200},{id:200,a:2,c:200},{id:200,c:200}];
   checkRec(actR, expR);
   println("---check replace shardingKey and other field, commit success");
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}
main()