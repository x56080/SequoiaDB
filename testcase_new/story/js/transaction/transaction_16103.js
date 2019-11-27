/************************************
*@Description: cs事务操作过程中，其他连接修改cs名
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16103
**************************************/

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

function main()
{
   println( "---begin rename cs test---" );
   var oldcsName = COMMCSNAME+"_16103_old";
   var newcsName = COMMCSNAME+"_16103_new";
   var clName = CHANGEDPREFIX + "_16103_cl";
   
   commDropCS( db, oldcsName );
   commDropCS( db, newcsName );
   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCLByOption( db, oldcsName, clName, {}, false, false, "create CL in the begin" );
   
   //insert 1000 data
   insertData( cl, 1000 );
   
   db.transBegin();
   cl.insert( {"no":10086, customerName:"testTrans", "phone":13700010086, "openDate":1402990912105} );
   cl.insert( {"no":10000, customerName:"testTrans", "phone":13700010000, "openDate":1402990912106} );
   
   var newdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   try
   {
      newdb.renameCS( oldcsName, newcsName );
   }
   catch( e )
   {
      if( e !== -190 )
      {
         throw new Error( e );
      }
   }
   
   db.transCommit();
   
   checkCount( cl, 2, {customerName: 'testTrans'} );
   
   checkRenameCSResult( newcsName, oldcsName, 1 );
   
   commDropCS( db, oldcsName, true, false, "clean cs---" );
   println( "---end the test---" );
}
