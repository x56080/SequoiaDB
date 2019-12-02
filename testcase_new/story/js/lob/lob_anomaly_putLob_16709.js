/******************************************************************************
*@Description: test putLob with specify oid
*@author:      wangkexin
*@createdate:  2018.11.23
*@testlinkCase: seqDB-16709:putLob，指定oid插入大对象
******************************************************************************/

main( db ); 
function main( db )
{
   var clName = CHANGEDPREFIX + "_putlob16709"; 
   var testFile = CHANGEDPREFIX + "_lobTest16709.file"; 
   var getTestFile = CHANGEDPREFIX + "_lobTestGet16709.file"; 
   var cmd = new Cmd(); 
   var oid = "5bf7575bdc4e88fa3dd16709"; 
   
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" ); 
   
   lobGenerateFile( testFile ); 
   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, true, "create collection" ); 
   var md5Arr = cmd.run( "md5sum " + testFile ).split( " " ); 
   var md5 = md5Arr[0]; 
   
   println( "begin to put lob with specify oid" )
   // put lob with specify oid
   try
   {
      cl.putLob( testFile, oid ); 
      println( "success to put lob in colleciton" ); 
   }
   catch( e )
   {
      throw buildException( "check put lob with specify oid ", e ); 
   }
   
   try
   {
      cl.getLob( oid, getTestFile, true ); 
      md5Arr = cmd.run( "md5sum " + getTestFile ).split( " " ); 
      getMd5 = md5Arr[0]; 
      if( getMd5 !== md5 )
      {
         println( "put lob file md5: " + md5 ); 
         println( "get lob file md5: " + getMd5 ); 
         throw "NotEqualMd5"; 
      }
      println( "success to get lob in colleciton" ); 
      // delete lob
      cl.deleteLob( oid ); 
      println( "success to delete lob in colleciton" ); 
   }
   catch( e )
   {
      throw buildException( "check get lob with specify oid ", e ); 
   }
   finally
   {
      cmd.run( "rm -rf " + testFile ); 
      cmd.run( "rm -rf " + getTestFile ); 
      commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end, error" ); 
   }
}

