/******************************************************************************
@Description :   seqDB-17069:校验事务增删改操作的提交与回滚
@Modify list :   2019-1-9    xiaoni Zhao  Init
******************************************************************************/
function main()
{
   var clName = COMMCLNAME + "_17069";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   db.transBegin();
   dbcl.insert( { transaction: 1 } );
   db.transCommit();
   checkCount( dbcl, 1, {transaction : 1} );
   
   db.transBegin();
   dbcl.insert( { transaction: 2 } );
   db.transRollback();
   checkCount( dbcl, 0, {transaction : 2} );
   
   db.transBegin();
   dbcl.update( {$set:{transaction : -1}} );
   db.transCommit();
   checkCount( dbcl, 1, {transaction : -1} );
   
   db.transBegin();
   dbcl.update( {$set:{transaction : 1}} );
   db.transRollback();
   checkCount( dbcl, 0, {transaction : 1} );
   
   db.transBegin();
   dbcl.remove();
   db.transRollback();
   checkCount( dbcl, 1 );
   
   db.transBegin();
   dbcl.remove();
   db.transCommit();
   checkCount( dbcl, 0 );
   
   commDropCL( db, COMMCSNAME, clName, true, true );
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
