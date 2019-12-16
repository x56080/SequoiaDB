/************************************
*@Description: 事务操作过程中修改cs名
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16105
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


function main ()
{
   println( "---begin rename cs test---" );
   var oldcsName = COMMCSNAME + "_16105_old";
   var newcsName = COMMCSNAME + "_16105_new";
   var clName = CHANGEDPREFIX + "_16105_cl";
   commDropCS( db, oldcsName );
   commDropCS( db, newcsName );
   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   db.transBegin();
   cl.insert( { "no": 10086, customerName: "testTrans", "phone": 13700010086, "openDate": 1402990912105 } );
   cl.insert( { "no": 10000, customerName: "testTrans", "phone": 13700010000, "openDate": 1402990912106 } );

   for( var i = 0; i < 10; i++ )
   {
      try
      {
         db.renameCS( oldcsName, newcsName );
         throw "rename cs in trans should be fail!";
      }
      catch( e )
      {
         if( e !== -336 )
         {
            throw new Error( e );
         }
      }
   }

   cl.insert( { "no": 10001, customerName: "testTrans", "phone": 13700010001, "openDate": 1402990912107 } );
   cl.insert( { "no": 10100, customerName: "testTrans", "phone": 13700010248, "openDate": 1402990912108 } );

   db.transCommit();

   db.renameCS( oldcsName, newcsName );

   cl = db.getCS( newcsName ).getCL( clName );
   checkCount( cl, 4, { customerName: 'testTrans' } );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   commDropCS( db, newcsName, true, false, "clean cs---" );
   println( "---end the test---" );
}

