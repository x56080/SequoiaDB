/*******************************************************************************
*@Description:   seqDB-12792: headerline和fields同时存在 
*@Author:        2019-3-5  wangkexin
********************************************************************************/

var csvContent1 = 'NAME,ID\ntest1,123\ntest2,123\ntest3,123' + "\n" ;
var csvContent2 = 'NAME|ID\ntest1|123\ntest2|123\ntest3|123' + "\n" ;
var csvContent3 = 'NAME|ID\ntest1,123\ntest2,123\ntest3,123' + "\n" ;
main();
function main()
{
    try
    {
        var csName = COMMCSNAME;
        var clName = COMMCLNAME+"_12792" ;
        var cl = readyCL( csName, clName );
        
        //准备数据文件
        var imprtFile1 = tmpFileDir +"12792a.csv";
        readyData( imprtFile1, csvContent1);
        var imprtFile2 = tmpFileDir +"12792b.csv";
        readyData( imprtFile2, csvContent2);
        var imprtFile3 = tmpFileDir +"12792c.csv";
        readyData( imprtFile3, csvContent3);
        //1. 指定headerline=true，field字段，不指定分隔符  注：前5种情况预期结果相同，故只定义一次expRecs
        importData( csName, clName, imprtFile1, true, 3, 0, 3);
        var expRecs = '[{"yourname":"test1","yourid":123},{"yourname":"test2","yourid":123},{"yourname":"test3","yourid":123}]';
        checkCLData( cl, expRecs, 3);
        
        //2. 指定headerline=true，field字段，指定相同分隔符
        importData( csName, clName, imprtFile2, true, 3, 0, 3, '|');
        checkCLData( cl, expRecs, 3);
        
        //3. 指定headerline=true，field字段，指定与headerline不同的分隔符
        importData( csName, clName, imprtFile3, true, 3, 0, 3, ',');
        checkCLData( cl, expRecs, 3);
        
        //4. 指定headerline=false，field字段，不指定分隔符
        importData( csName, clName, imprtFile1, false, 3, 1, 3 );
        checkCLData( cl, expRecs, 3);
        
        //5. 指定headerline=false，field字段，指定相同分隔符
        importData( csName, clName, imprtFile2, false, 3, 1, 3, '|');
        checkCLData( cl, expRecs, 3);
        
        //6. 指定headerline=false，field字段，指定与headerline不同的分隔符
        importData( csName, clName, imprtFile3, false, 4, 0, 4, ',');
        var expRecs = '[{"yourname":"NAME|ID"},{"yourname":"test1","yourid":123},{"yourname":"test2","yourid":123},{"yourname":"test3","yourid":123}]';
        checkCLData( cl, expRecs, 4);
        
        cleanCL( csName, clName );
    }
    catch(e)
    {
        throw e;
    }
}

function readyData( imprtFile, csvContent )
{
    println("---Begin to ready data.");
    
    var file = fileInit( imprtFile );
    file.write(csvContent);
    file.close();
}

function importData( csName, clName, imprtFile, headerline, expParseRecordsNum, expParseFailureNum, expImportedRecordsNum, delfield )
{
    println("\n---Begin to import data and check exec result.");
    
    //remove rec file
    var tmpRec = csName +"_"+ clName +"*.rec";
    cmd.run( "rm -rf "+ tmpRec );
    
    //import operation
    if ( typeof delfield == "undefined" )
    {
        var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                    +' -c '+ csName +' -l '+ clName 
                    +' --type csv '
                    +' --file '+ imprtFile
                    +' --fields "yourname string,yourid int"'
                    +' --headerline ' + headerline;
    }
    else
    {
        var imprtOption = installDir +'bin/sdbimprt -s '+ COORDHOSTNAME +' -p '+ COORDSVCNAME 
                    +' -c '+ csName +' -l '+ clName 
                    +' --type csv '
                    +' --file '+ imprtFile
                    +' -e "'+delfield+'"'
                    +' --fields "yourname string,yourid int"'
                    +' --headerline ' + headerline;
    }
    var rc = cmd.run( imprtOption );
    
    //check import results
    var rcObj = rc.split("\n");
    var expParseRecords    = "parsed records: "+expParseRecordsNum;
    var expParseFailure    = "parse failure: "+expParseFailureNum;
    var expImportedRecords = "imported records: "+expImportedRecordsNum;
    var actParseRecords    = rcObj[0];
    var actParseFailure    = rcObj[1];
    var actImportedRecords = rcObj[4];
    if( expParseRecords !== actParseRecords 
        || expParseFailure !== actParseFailure
        || expImportedRecords !== actImportedRecords)
    {
        throw buildException( "importData", null, "[sdbimprt results]", 
                        "["+ expParseRecords +", "+ expImportedRecords +"]", 
                        "["+ expParseFailure +", "+ actParseFailure +"]", 
                        "["+ actParseRecords +", "+ actImportedRecords +"]" );
    }
    
    // clean tmpRec
    cmd.run( "rm -rf " + tmpRec );
}

function checkCLData( cl, expRecs, expCnt )
{
    println("---Begin to check cl data.");
    
    var rc = cl.find({},{_id:{$include:0}}).sort({"yourname":1});
    var recsArray = [];
    while( tmpRecs = rc.next() )
    {
        recsArray.push( tmpRecs.toObj() );
    }
    
    rc.close();
    var actCnt  = recsArray.length;
    var actRecs = JSON.stringify( recsArray );
    if( actCnt !== expCnt || actRecs !== expRecs )
    {
        throw buildException( "checkCLdata", null, "[find]", 
                        "[cnt:"+ expCnt +", recs:"+ expRecs +"]", 
                        "[cnt:"+ actCnt +", recs:"+ actRecs +"]" );
    }
    //println( "cl records: "+ actRecs );
    cl.truncate();
}