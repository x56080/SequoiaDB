/****************************************************
@description:   split, abnormal case
                testlink cases: seqDB-7224
@input:         insert a rec: {age:1}, split by [splitpercent=50]
@expectation:   1 lackSource(): expect: return -6 
                2 lackTarget(): expect: return -6 
                3 lackSpiltpercent(): expect: return -6 
@modify list:
                2015-4-7 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7224";
var sourceGroup;
var targetGroup;

function lackSource ()
{
   var word = "split";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName,
   // 'source='+sourceGroup, 
   'target=' + targetGroup, 'splitpercent=50'], [-6], "error occurs in " + getFuncName() );
}

function lackTarget ()
{
   var word = "split";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'source=' + sourceGroup,
      // 'target='+targetGroup,
      'splitpercent=50'], [-6], "error occurs in " + getFuncName() );
}

function lackSplitpercent ()
{
   var word = "split";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'source=' + sourceGroup, 'target=' + targetGroup,
      // 'splitpercent=50'
   ], [-6], "error occurs in " + getFuncName() );
}


function main ()
{
   var groupInfos = commGetGroups( db );
   if( commIsStandalone( db ) || groupInfos.length < 2 )
   {
      println( "Mode is standalone or one group mode" );
      return;
   }
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   sourceGroup = groupInfos[0][0].GroupName;
   targetGroup = groupInfos[1][0].GroupName;
   var opt = { ShardingKey: { age: 1 }, ShardingType: "range", Group: sourceGroup };
   var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
   varCL.insert( { age: 1 } );
   lackSource();
   lackTarget();
   lackSplitpercent();
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
