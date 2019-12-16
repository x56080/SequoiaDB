/******************************************************************************
*@Description : test remove special decimal value
*               seqDB-13992:删除特殊decimal值             
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME );
   insertData( cl, docs );

   for( var i = 0; i < docs.length; i++ )
   {
      deleteData( cl, docs[i] );
      var docNum = cl.count( docs[i] );
      if( parseInt( docNum ) !== 0 )
      {
         throw buildException( "main", null, "check doc num", 0, docNum );
      }
   }
}