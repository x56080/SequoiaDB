/******************************************************************************
*@Description: test putLob with incorrect oid
*@author:      wangkexin
*@createdate:  2018.11.20
*@testlinkCase: seqDB-16583:putLob,oid参数取值格式校验
******************************************************************************/

function main( db )
{
	//clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,"drop CL in the beginning" ) ;
   
   var testFile = CHANGEDPREFIX + "_lobTest16583.file" ;
   lobAutoFile( testFile ) ;
   
   var incorrectOid = "123456";
   
   // create collection
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, -1, true, true, true,
                          "create collection" ) ;
   // put lob with incorrect oid
   
   try
   {
      cl.putLob(testFile, incorrectOid) ;
      throw "ErrExecuteLob" ;
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "failed to execute put lob with incorrect oid, rc = " + e ) ;
         throw e ;
      }
      else
         println( "success to execute putLob with incorrect oid" ) ;
   }finally
   {
      var cmd = new Cmd() ;
      // remove lobfile
      cmd.run( "rm -rf " + testFile ) ;
   }
}

// Run Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "clear collection in the beginning" ) ;
   main( db ) ;
}
catch( e )
{
   throw e ;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop collection in the end , error" ) ;
   db.close( ) ;
}
