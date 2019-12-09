/******************************************************************************
*@Description : test Oma function: setIniConfigs getIniConfigs                                       
*               seqDB-17947:用户级配置配置AuditMask掩码 
*@Author      : 2019-1-28  XiaoNi Huang
*@Info        ：审计日志需要在日志文件查看，自动化用例不好实现，现自动化只在编目SYSCAT.SYSUSERS查看配置的掩码有写到用户系统表即可
******************************************************************************/
main();

function main ()
{
   var user = "user17947";

   try
   {

      // create user
      println( "\n---Begin to create user[" + user + "]" );
      var auditmask = "!ACCESS|!CLUSTER|!SYSTEM|DML|DDL|DCL|DQL|INSERT|UPDATE|DELETE|OTHER";
      db.createUsr( user, user, { AuditMask: auditmask } );

      // connect catalog, check results
      println( "\n---Begin to check results" );
      var cdb = new Sdb( COORDHOSTNAME, CATASVCNAME, user, user );
      var rc = cdb.SYSAUTH.SYSUSRS.find( { "User": user } );
      var rcAuditMask = rc.current().toObj()["Options"]["AuditMask"];
      rc.close();

      if( auditmask !== rcAuditMask )
      {
         throw buildException( "checkResult", null, "[checkResult]", auditmask, "  " + rcAuditMask );
      }
   }
   catch( e )
   {
      throw e;
   }
   finally 
   {
      println( "\n---Begin to drop user[" + user + "]" );
      db.dropUsr( user, user );
   }
}