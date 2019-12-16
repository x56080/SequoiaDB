/******************************************************
 * @Description: seqDB-10429:未attach子表的主表做数据插入
 * @Author: linsuqiang 
 * @Date: 2016-11-23
 ******************************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }

   var csName = COMMCSNAME + "_cs";
   var mainCLName = COMMCLNAME + "_mcl";

   println( "\n---Begin to drop cs in the pre-condition." );
   commDropCS( db, csName, true, "Failed to drop CS." );

   db.setSessionAttr( { PreferedInstance: "M" } );
   commCreateCS( db, csName, false, "Failed to create CS." );
   var mainCL = createMainCL( csName, mainCLName );

   insertRec( mainCL );

   println( "\n---Begin to drop cs/domain in the end-condition." );
   commDropCS( db, csName, false, "Failed to drop CS." );

}

function createMainCL ( csName, mainCLName )
{
   println( "\n---Begin to create MainCL." );

   var options = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function insertRec ( mainCL )
{
   try
   {
      println( "\n---Begin to insert records." );
      mainCL.insert( { a: "adfadfadf", b: "ijijkkkijikji" } );
   }
   catch( e )
   {
      var exceptE = -135;
      if( e !== exceptE )
      {
         throw buildException( "insertRecOutBount", null, "[check Error Code]",
            "[" + exceptE + "]",
            "[" + e + "]" );
      }
   }
}

