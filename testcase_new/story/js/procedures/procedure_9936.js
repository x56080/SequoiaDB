/* *****************************************************************************
@Description:   seqDB-9936:执行索引相关的存储过程
@modify list:
         
***************************************************************************** */
function main ( db )
{
   var procName = new Array( CHANGEDPREFIX + "_createCS_CL", CHANGEDPREFIX + "_createIndex",
      CHANGEDPREFIX + "_getIndex", CHANGEDPREFIX + "_dropIndex" );
   var idxName = CHANGEDPREFIX + "_procIdx";
   var idxKey = "idNum";
   fmpRemoveProcedures( procName, true );
   println( "success to remove procedures in the beginning" );
   var index = 0;
   try
   {
      eval( "db.createProcedure( function " + procName[index] + "(csName,clName){" +
         "try" +
         "{" +
         "   db.dropCS(csName) ;" +
         "}" +
         "catch ( e )" +
         "{" +
         "   if (-34 !=e )" +
         "   {" +
         "      println(\"Failed to drop collection space\"+e) ;" +
         "      throw e ;" +
         "   }" +
         "}" +
         "try" +
         "{" +
         "   var cs = db.createCS(csName) ;" +
         "   var cl = cs.createCL(clName, {ReplSize:0} ) ;" +
         "}" +
         "catch ( e )" +
         "{" +
         "   println(\"Failed to create collection space\"+e) ;" +
         "   throw e ;" +
         "}" +
         "return cl ;" +
         "}" +
         ")"
      );

      ++index;
      eval( "db.createProcedure(function " + procName[index] +
         "(csName, clName, idxName, idxKey ){" +
         "var cl = " + procName[0] + "( csName, clName ) ;" +
         "try" +
         "{" +
         "   cl.dropIndex( idxName ) ;" +
         "}" +
         "catch ( e )" +
         "{" +
         "   if ( -47 != e )" +
         "   {" +
         "      println(\"Failed to drop index,begin\"+e) ;" +
         "      throw e ;" +
         "   }" +
         "}" +
         "try" +
         "{" +
         "   cl.createIndex( idxName, { idxKey : -1 } ) ;" +
         "}" +
         "catch ( e )" +
         "{" +
         "   println(\"Failed to create index\"+e) ;" +
         "   throw e ;" +
         "}" +
         "})"
      );

      ++index;
      eval( "db.createProcedure(function " + procName[index] +
         "( csName, clName, idxName, idxKey ){" +
         "try" +
         "{" +
         "   var cl = " + procName[0] + "( csName, clName ) ;" +
         procName[1] + "( csName, clName, idxName, idxKey) ;" +
         "   var cnt = 0 ;" +
         "   var noSync = false ;" +
         "   do{ try {" +
         "      cl.insert( {\"note\":\"make sure all nodes have cs\"} ) ;" +
         "      var getIdx = cl.getIndex( idxName ) ;" +
         "      if( undefined == getIdx )" +
         "         noSync = true ;" +
         "      else" +
         "         noSync = false ;" +
         "   }catch(e){" +
         "      if( -23 == e || -34 == e )" +
         "      {" +
         "         ++cnt ;" +
         "         noSync = true ;" +
         "         if( cnt >= 1000 )" +
         "            break ;" +
         "      }" +
         "      else" +
         "      {" +
         "         throw e ;" +
         "         break ;" +
         "      }" +
         "   }}while( true == noSync ) ;" +
         "   cl.getIndex( idxName ) ;" +
         "}" +
         "catch ( e )" +
         "{" +
         "   throw e ;" +
         "}" +
         "})"
      );

      ++index;
      eval( "db.createProcedure(function " + procName[index] +
         "( csName, clName, idxName ){" +
         "try" +
         "{" +
         "   var cl = " + procName[0] + "( csName, clName ) ;" +
         "   cl.dropIndex( idxName ) ;" +
         "}" +
         "catch ( e )" +
         "{" +
         "   if (  -34!=e )" +
         "   {" +
         "      println(\"Failed to drop Index,function\"+e) ;" +
         "      throw e ;" +
         "   }" +
         "}" +
         "})"
      );
   }
   catch( e )
   {
      println( "create procedure failed, rc = " + e );
      throw e;
   }

   try
   {
      db.listProcedures();
   }
   catch( e )
   {
      println( "failed to list all procedures, rc = " + e );
      throw e;
   }

   try
   {
      // list create index procedure
      db.listProcedures( { name: procName[1] } );
   }
   catch( e )
   {
      println( "failed to list procedrue createIndex, rc = " + e );
      throw e;
   }
   println( "list procedures success" );

   try
   {
      // list create cs and cl procedure
      db.listProcedures( { name: procName[0] } );
   }
   catch( e )
   {
      println( "failed to list procedure createCS_CL, rc = " + e );
      throw e;
   }
   // run create index procedure
   println( "begin to eval procedures" );
   try
   {
      index = 1; // createIndex
      db.eval( procName[index] + "(\"" + COMMCSNAME + "\", \"" + COMMCLNAME +
         "\", \"" + idxName + "\", \"" + idxKey + "\")" );
   }
   catch( e )
   {
      println( "failed to eval createCS_CL, rc = " + e );
      throw e;
   }
   println( "eval procedures successful" );
   try
   {
      index = 2; // get index
      println( "begin to get index in procedures" );
      db.eval( procName[index] + "(\"" + COMMCSNAME + "\",\"" +
         COMMCLNAME + "\",\"" + idxName + "\", \"" + idxKey + "\")" );
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
         "failed to drop collection" );
      println( "get index in procedure success" );
   }
   catch( e )
   {
      println( "failed to eval get index, rc = " + e );
      throw e;
   }

   try
   {
      index = 2; // get index
      db.removeProcedure( procName[index] );
   }
   catch( e )
   {
      println( "Failed to remove 'getIndex'" + e );
      throw e;
   }

   try
   {
      index = 3; // drop index
      db.removeProcedure( procName[index] );
   }
   catch( e )
   {
      println( "Failed to remove 'dropIndex'" + e );
      throw e;
   }
   fmpRemoveProcedures( procName, true );
   println( "success to remove procedures in the end" );
}

try
{
   if( false == commIsStandalone( db ) )
      main( db );
   else
      println( "run mode : standalone" );
}
catch( e )
{
   throw e;
}
