/***************************************************************************
@Description : seqDB-15725:指定快照查询参数查询节点健康检测快照
@Modify list : 2019-11-14  Chen siqin  Create
****************************************************************************/
function main()
{
   var option = new SdbSnapshotOption().cond({$and:[{IsPrimary:true}, {ServiceStatus:true}]}).sel({IsPrimary:1}).limit(1);
   var cursor = db.snapshot(SDB_SNAP_HEALTH, option);
   var actResult = [];
   var expResult = [{"IsPrimary": true}];
   while( cursor.next() )
   {
      actResult.push(cursor.current().toObj());
   }
   checkResult(actResult, expResult);
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
   throw e;
}
