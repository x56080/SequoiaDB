/************************************
*@Description: 查询regex类型数据，参数错误 
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-11080
**************************************/
main();
function main ()
{
   var clName = "cl11080";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning." );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //查询regex类型数据，$options值错误
   try
   {
      var rc = cl.find( { regex: { $regex: "aaa", $options: 1 } } ).toArray();
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "main()", e, "$options value is wrong", -6, e );
      }
   }

   //查询regex类型数据，带非 $options 的其他参数
   try
   {
      var rc = cl.find( { regex: { $regex: "aaa", a: "1" } } ).toArray();
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "main()", e, "query without $options", -6, e );
      }
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the end." );
}
