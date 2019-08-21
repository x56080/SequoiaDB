/******************************************************************************
@Description: seqDB-10442:数据落在相同组上的相同子表中，批量插入数据
@modify list:
   2016.11.24  zengxianquan  Init
   2019-4-15   xiaoni huang  modify
*******************************************************************************/

main();
function main()
{
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   } 
   if ( commGetGroupsNum( db ) < 1 )
   {
      println("---Least two groups");
      return ;
   }
   db.setSessionAttr( { PreferedInstance: "M" } );
   
   var mclName  = "mcl_10442" ;
   var sclName1 = "scl_10442_1" ;
   var sclName2 = "scl_10442_2" ;
   var groups = commGetGroups(db, false, "", false, true, true );
   var rgName = groups[1][0].GroupName;

   commDropCL( db, COMMCSNAME, mclName, true, true, "clean main cl" );
   commDropCL( db, COMMCSNAME, sclName1, true, true, "clean sub cl1" );
   commDropCL( db, COMMCSNAME, sclName2, true, true, "clean sub cl2" );
   
   // create main cl
   println("\n---Begin to create cl.");
   var mOpt = { ShardingKey:{a:1}, IsMainCL:true };
   var mainCL = commCreateCLByOption( db, COMMCSNAME, mclName, mOpt, true, true );
   // create sub cl
   var sOpt = { ShardingKey:{ a:1 }, ShardingType: "hash", ReplSize:0, Compressed:true, Group: rgName };
   commCreateCLByOption( db, COMMCSNAME, sclName1, sOpt, true, true );
   var sOpt = { ShardingKey:{ a:1 }, ShardingType: "range", ReplSize:0, Compressed:true, Group: rgName };
   commCreateCLByOption( db, COMMCSNAME, sclName2, sOpt, true, true );
   // attach cl
   println("\n---Begin to attach cl.");
   mainCL.attachCL( COMMCSNAME + "." + sclName1, { LowBound:{a:0},   UpBound:{a:1000} } ) ;
   mainCL.attachCL( COMMCSNAME + "." + sclName2, { LowBound:{a:1000},UpBound:{a:2000} } ) ;
      
   // insert   
   println("\n---Begin to insert.");
   var recordsNum = 2000;
   var docs = [];
   for(var i = 0; i < recordsNum ; ++i )
   {
      docs.push( {a: i} );
   }
   mainCL.insert( docs );

   // check results
   println("\n---Begin to check total count.");
   var totalCnt = mainCL.count();
   if( Number( totalCnt ) !== recordsNum ) 
   {
      throw buildException( "main", null, "check total count", recordsNum, totalCnt );
   }
   
   // check rg count
   checkResults( rgName, sclName1, recordsNum );
   checkResults( rgName, sclName2, recordsNum );
   
   
   commDropCL( db, COMMCSNAME, mclName, true, true, "clean main cl in the end" );
}

function checkResults( rgName, sclName, recordsNum )
{   
   var sclFullName = COMMCSNAME + "." + sclName;
   println("\n---Begin to check " + sclFullName + " in rg.");
   var clRGs = db.snapshot(8, { Name: sclFullName }).current().toObj().CataInfo;
   if( clRGs.length !== 1 ) 
   {
      throw buildException( "main", null, "check subCL1 rgNum", 1, clRGs.length );
   }   
   var rg = db.getRG( rgName ).getMaster().connect();
   var cnt = rg.getCS( COMMCSNAME ).getCL( sclName ).count();
   var expCnt = recordsNum / 2;
   if( Number( cnt ) !== expCnt ) 
   {
      throw buildException( "main()", null, "check cl count in rg", expCnt, cnt );
   }
}