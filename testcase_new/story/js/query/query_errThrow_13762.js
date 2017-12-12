/*******************************************************************************
*@Description : when query like: db.foo.bar.find({$a:1}), we should throw error
*               -6 and print string: Invalid Argument
*               SEQUOIADBMAINSTREAM-512
*@Modify List :
*               2014-9-26   xiaojunHu  Init
*******************************************************************************/

function main( db )
{
   try
   {
      var installDir = commGetInstallPath() ;
      println( "install path: " + installDir ) ;
      var cmd = new Cmd() ;
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, true,
                             "create collection in the beginning" ) ;
      // insert record
      cl.insert( {a:1} ) ;
      cl.insert( {b:"testcase"} ) ;
      // query by use db.foo.bar.find({$a:1}).getLastErrMsg() will get the message
      try
      {
         println( cl.find( {$a: 1} ) ) ;
         throw "ErrExcuteTest" ;
      }
      catch( e )
      {
         var lastOut = getLastErrMsg() ;
         println( "Success to print last error msg: " + lastOut ) ;
         if( getErr( -6 ) != lastOut )
         {
            println( "failed to print correct error, rc = " + e ) ;
            throw e ;
         }
      }
   }
   catch( e )
   {
      throw e ;
   }
}

// run main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop collection in the beginning" ) ;
   main( db ) ;
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
               "drop collection in the beginning" ) ;
}
catch( e )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop collection in the beginning" ) ;
   throw e ;
}
