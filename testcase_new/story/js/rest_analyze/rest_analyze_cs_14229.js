/****************************************************
@description:     test analyze cs
@testlink cases:  seqDB-14229
@modify list:
2018-07-30        linsuqiang init
****************************************************/

var analyzeCsName = "analyze14229";
var options = { PageSize: 4096 };
commDropCS( db, analyzeCsName, /*ignoreNotExist*/true );
commCreateCS( db, analyzeCsName, false, "fail to create cl", options )
var nonAnalyzeCsName = "nonAnalyze14229";
commDropCS( db, nonAnalyzeCsName, /*ignoreNotExist*/true );
commCreateCS( db, nonAnalyzeCsName, /*ignoreExisted*/false, "fail to create cl", options );

var clNumPerCs = 2;
var analyzeClArray = [];
for( var i = 0; i < clNumPerCs; i++ )
{
   var cl = db.getCS( analyzeCsName ).createCL( analyzeCsName + "_" + i );
   insertDataWithIndex( cl );
   analyzeClArray.push( cl );
}
var nonAnalyzeClArray = [];
for( var i = 0; i < clNumPerCs; i++ )
{
   var cl = db.getCS( nonAnalyzeCsName ).createCL( nonAnalyzeCsName + "_" + i );
   insertDataWithIndex( cl );
   nonAnalyzeClArray.push( cl );
}

for( var i = 0; i < analyzeClArray.length; i++ )
   checkScanTypeByExplain( analyzeClArray[i], "ixscan" );
for( var i = 0; i < nonAnalyzeClArray.length; i++ )
   checkScanTypeByExplain( nonAnalyzeClArray[i], "ixscan" );
tryCatch( ["cmd=analyze", "options={CollectionSpace: \"" + analyzeCsName + "\"}"],
   [0],
   "fail to analyze." );
for( var i = 0; i < analyzeClArray.length; i++ )
   checkScanTypeByExplain( analyzeClArray[i], "tbscan" );
for( var i = 0; i < nonAnalyzeClArray.length; i++ )
   checkScanTypeByExplain( nonAnalyzeClArray[i], "ixscan" );

commDropCS( db, analyzeCsName, /*ignoreNotExist*/false );
commDropCS( db, nonAnalyzeCsName, /*ignoreNotExist*/false ); 
