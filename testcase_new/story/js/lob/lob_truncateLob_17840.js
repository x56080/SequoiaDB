/***************************************************************************
@Description :seqDB-17840: oid字符串参数校验
@Modify list :
2019-2-15  YinZhen  Create
****************************************************************************/
function main ()
{

   var clName = COMMCLNAME + "_17840";
   commDropCL( db, COMMCSNAME, clName, true, true );

   //create collection
   var dbcl = commCreateCL( db, COMMCSNAME, clName, 0 );

   //truncateLob oid string checked
   try
   {
      dbcl.truncateLob( "", 1 );
      throw "TRUNCATE LOB ERROR";
   }
   catch( e )
   {
      if( -6 != e )
      {
         throw buildException( "truncateLob()", "truncateLob", "truncate lob with wrong oid", -6, e );
      }
   }
   try
   {
      dbcl.truncateLob( "sas", 12 );
      throw "TRUNCATE LOB ERROR";
   }
   catch( e )
   {
      if( -6 != e )
      {
         throw buildException( "truncateLob()", "truncateLob", "truncate lob with wrong oid", -6, e );
      }
   }
   commDropCL( db, COMMCSNAME, clName, true, true );
}

main(); 
