/******************************************************************************
*@Description : seqDB-18270:原有接口基本功能验证 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName = "cl_18270";
   var indexName = "idx";

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false,
      "Failed to create CL." );

   println( "\n---Begin to create index." );
   var unique = true;
   var enforced = true;
   var sortBufferSize = 32;
   cl.createIndex( indexName, { a: 1 }, true, true, sortBufferSize );

   println( "\n---Begin to check results." );
   var NotNull = false;
   checkIndex( cl, indexName, unique, enforced, NotNull );

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expUni, expEnf, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actUni = indexDef.unique;
   var actEnf = indexDef.enforced;
   var actNot = indexDef.NotNull;
   if( actUni !== expUni || actEnf !== expEnf || actNot !== expNot )
   {
      var expResults = JSON.stringify( { unique: expUni, enforced: expEnf, NotNull: expNot } );
      var actResults = JSON.stringify( { unique: actUni, enforced: actEnf, NotNull: actNot } );
      throw buildException( "checkResult", null, "", expResults, "  " + actResults );
   }
}