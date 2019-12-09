/******************************************************************************
*@Description : test insert special decimal value to mainCL
*               seqDB-13999:垂直分区表插入特殊decimal值          
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }
   var groups = getGroupName( db );
   if( groups.length < 2 )
   {
      println( "At least two groups" );
      return;
   }

   // test main cl
   var csName = COMMCSNAME + "_maincs13999";
   var mainClName = COMMCLNAME + "_maincl13999";
   var subClName1 = COMMCLNAME + "_subcl1";
   var subClName2 = COMMCLNAME + "_subcl2";
   commDropCL( db, csName, mainClName, true, true, "drop CL in the beginning" );
   commDropCL( db, csName, subClName1, true, true, "drop CL in the beginning" );
   commDropCL( db, csName, subClName2, true, true, "drop CL in the beginning" );

   var option = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0 };
   var mainCl = commCreateCLByOption( db, csName, mainClName, option, true, true );
   option = { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0 };
   var subCl1 = commCreateCLByOption( db, csName, subClName1, option, true, true );
   var subCl2 = commCreateCLByOption( db, csName, subClName2, option, true, true );
   var attachOption = { LowBound: { a: { $decimal: "MIN" } }, UpBound: { a: { $decimal: "0" } } };
   attachCL( mainCl, csName + "." + subClName1, attachOption );
   attachOption = { LowBound: { a: { $decimal: "0" } }, UpBound: { a: { $decimal: "MAX" } } };
   attachCL( mainCl, csName + "." + subClName2, attachOption );

   try
   {
      mainCl.insert( { a: { $decimal: "MAX" } } );
      throw 0;
   }
   catch( e )
   {
      if( e !== -135 )
      {
         throw buildException( "main", e, "insert MAX", -135, e );
      }
   }

   var docs = [{ a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];
   insertData( mainCl, docs );
   var cursor = sortFindData( subCl1, {}, {}, { _id: 1 } );
   checkRec( cursor, docs );

   // drop cs
   commDropCS( db, csName, true, "drop CS in the end" );
}