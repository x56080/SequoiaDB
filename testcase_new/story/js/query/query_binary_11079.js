/************************************
*@Description:  查询binary类型数据，参数错误
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-11079
**************************************/
main();
function main ()
{
   var clName = "cl11079";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning." );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   cl.insert( { "a": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   //查询binary类型数据，$type值错误 
   try
   {
      var rc = cl.find( { a: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1000" } } ).toArray();
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "main()", e, "$type value is wrong", -6, e );
      }
   }

   //查询binary类型数据，缺少$type 
   try
   {
      var rc = cl.find( { bindata: { "$binary": "aGVsbG8gd29ybGQ=" } } ).toArray();
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "main()", e, "query without $type", -6, e );
      }
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the end." );
}