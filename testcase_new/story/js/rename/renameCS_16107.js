/************************************
*@Description: 指定cs创建存储过程，修改cs名 
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16107
**************************************/

main();

function main ()
{
   //@ clean before
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var oldcsName = CHANGEDPREFIX + "_16107_oldcs";
   var newcsName = CHANGEDPREFIX + "_16107_newcs";
   var clName = CHANGEDPREFIX + "_16107_cl1";
   var procedureName = CHANGEDPREFIX + "_pro_16107";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine", "" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   var str = "db.createProcedure(function " + procedureName + "(){var cl = db.getCS('" + oldcsName + "').getCL('" + clName + "'); " +
      "cl.insert({'no':10086, 'customerName':'testTrans', 'phone':13700010086, 'openDate':1402990912105}) })";

   println( "---create procedure---" );
   db.eval( str );
   db.eval( procedureName + "()" );

   var num = cl.count();
   if( num != 1 )
   {
      throw buildException( "check cl record num", "", "procedure", 1, num );
   }

   println( "---rename cs---" );
   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   try
   {
      db.eval( procedureName + "()" );
      throw "PROCEDURE_SHOULD_ERR";
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw buildException( "check procedure after rename cs", e, "procedure", -34, e );
      }
   }

   println( "---remove procedure---" );
   db.removeProcedure( procedureName );

   var cur = db.listProcedures( { name: procedureName } );
   if( cur.next() != null )
   {
      throw buildException( "check list procedure", "", "procedure", "listProcedures is null", cur.current() );
   }

   commDropCS( db, newcsName, true, "clean cs---" );
}
