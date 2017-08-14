/******************************************************
 * @Description: seqDB-10445:主子表使用选择符过滤数据 
 * @Author: linsuqiang 
 * @Date: 2016-11-29
 ******************************************************/

main();

function main()
{
   if( commIsStandalone( db ) )
   {
      println(" Deploy mode is standalone!");
      return;
   }

   //splitting need two groups at least
   allGroupName = getGroupName( db );
   if( 1 === allGroupName.length ) {
      println("\n---one Group cannot support splitting");
      return ;
   }

   var csName = COMMCSNAME;
   var mainCLName = COMMCLNAME + "_mcl" ;

   // unset variable
   commDropCL( db, csName, mainCLName, true, true,"Fail to drop CL in the beginning" ) ;
   // create mainCL and subCL
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCL = createMainCL( csName, mainCLName );
   var subCLName = COMMCLNAME + "_scl";
   var lowBound = -3000;
   var upBound = 5000;
   createSubCL( csName, subCLName );
   attachCL( csName, mainCL, subCLName, lowBound, upBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, subCLName, startCondition, null );

   println("\n---Begin to insert records.");
   var recs = [ { _id: 1,
                  MCLKEY: 1, 
                  arr: [{ int: 1 },
                        { int: 2 },
                        { int: 3 },
                        { int: 4 },
                        { int: 4 } ],
                  dat: { $date: "2008-08-08" } } ];
   insertRecs( mainCL, recs );

   println("\n---Begin to test $include.");
   var includeOption = { dat: { $include: 1 } };
   var includeExpRes = [ { dat: { $date: "2008-08-08" } } ];
   var foundRes = testSel( mainCL, includeOption );
   checkFound( mainCL, foundRes, includeExpRes );

   println("\n---Begin to test $default.");
   var defaultOption = { str: { $default: "hahahahahaha" } };
   var defaultExpRes = [ { str: "hahahahahaha" } ];
   var foundRes = testSel( mainCL, defaultOption );
   checkFound( mainCL, foundRes, defaultExpRes );

   println("\n---Begin to test $elemMatchOne.");
   var elemOption = { arr: { $elemMatchOne: { int: 4 } } };
   var elemExpRes = [ { _id: 1,
                        MCLKEY: 1, 
                        arr: [{ int: 4 }],
                        dat: { $date: "2008-08-08" } } ];
   var foundRes = testSel( mainCL, elemOption );
   checkFound( mainCL, foundRes, elemExpRes );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL( csName, mainCLName )
{   
   var options = { ShardingKey: { MCLKEY: 1 }, ShardingType: "range", IsMainCL: true } ;
   var mainCL = commCreateCLByOption( db, csName, mainCLName, options, false, 
                                      true, "Failed to create mainCL." );                                  
   return mainCL ; 
}

function createSubCL( csName, subCLName )
{
   var options = { ShardingKey: { SCLKEY: 1 }, ShardingType:  "hash" };
   subCL = commCreateCLByOption( db, csName, subCLName, options, false, true, "Failed to create subCL." );
   return subCL;
}

function attachCL( csName, mainCL, subCLName, lowBound, upBound )
{
   var options = { LowBound: { MCLKEY: lowBound }, UpBound: { MCLKEY: upBound } };
   try
   {
      mainCL.attachCL( csName + "." + subCLName, options );
   }
   catch( e )
   {
      throw buildException( "attachCL", null, "", "", "" + e );
   }
}

function insertRecs( mainCL, recs )
{
   insertValidRecs( mainCL, recs );
}

function testSel( mainCL, option )
{
   var foundRes;
   try
   {
      foundRes = mainCL.find( { MCLKEY: 1}, option ).sort( { _id: 1 } );
   }
   catch( e )
   {
      throw buildException( "testSel", null, "", "", "" + e );
   }
   return foundRes;
}

function checkFound( mainCL, foundRes, foundExpRes )
{
   lsqCheckRec( foundRes, foundExpRes);
}