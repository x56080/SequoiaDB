function getRandomInt( min, max ) // [min, max)
{
    var range = max - min;
    var value = min + parseInt( Math.random() * range );
    return value;
}

function getRandomString( strLen ) //string length value locate in [minLen, maxLen)
{
    var str = "";
    for( var i = 0; i < strLen; i++ )
    {
       var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
       var c = String.fromCharCode( ascii );
       str += c;
    }
    return str;
}

function checkSortResultForLargeData(cursor, sortCond)
{
    while(cursor.next())
    {
       var expectResult = cursor.current().toObj(); // the pre one
       if(cursor.next())
       {
           var actResult = cursor.current().toObj(); // the next one
           for(var sortKey in sortCond)
           {
               if(expectResult[sortKey] > actResult[sortKey])  // sort result not expected
               {
                   throw buildException("checkSortResultForLargeData", "check result", "check result",
                                               JSON.stringify(expectResult), JSON.stringify(actResult));
               }
               else if(expectResult[sortKey] < actResult[sortKey]) // compare next record
               {
                   break;
               }
           }
       }
    }
    println("-----check sort result success-----");
}
