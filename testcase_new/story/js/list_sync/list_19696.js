/******************************************************************************
*@Description : seqDB-19696:检查list( SDB_LIST_BACKUPS )列表信息
*               TestLink : seqDB-19696
*@author      : wangkexin
*@Date        : 2019-09-27
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "------Deploy is standalone" );
      return;
   }

   var groupsArray = commGetGroups( db, false, "", true, true, false );
   var rgName = groupsArray[0][0].GroupName;
   var backupName = "backup19696";

   //backup group data 
   println( "begin to backup data group: " + rgName );
   db.backup( { Name: backupName, GroupName: rgName } );

   //check list result
   var fields = ["Version", "Name", "ID", "NodeName", "GroupName", "EnsureInc", "BeginLSNOffset", "EndLSNOffset", "TransLSNOffset", "StartTime", "LastLSN", "LastLSNCode", "HasError"];
   var cur = db.list( SDB_LIST_BACKUPS, { "Name": backupName } );
   while( cur.next() )
   {
      var ret = cur.current().toObj();
      for( var index in fields )
      {
         var field = fields[index];
         if( ret[field] == null )
         {
            throw new Error( "compare field failed, exp field  [" + field + "] is not null, but found null : ret[field] = " + ret[field] );
         }
      }
   }
   db.removeBackup( { Name: backupName } );
}

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}