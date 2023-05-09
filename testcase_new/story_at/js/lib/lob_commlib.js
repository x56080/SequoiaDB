/******************************************************************************
*@Description : Test the hint index common function.
*@Modify list :
*               2023-05-05   Yang Qincheng  Init
******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/******************************************************************************
 * @description: 向指定 cl 中插入指定数量数据
 * @param {string} cl      //  集合名
 * @param {int} recordNum  // 数据条数
 ******************************************************************************/
function insertDoc ( cl, recordNum )
{
   var docs = [];
   for( var i = 0; i < recordNum; ++i )
   {
      docs.push(
         {
            no: i, score: i, interest: ["movie", "photo"],
            major: "计算机软件与理论", dep: "计算机学院",
            info: { name: "Holiday", age: 22, sex: "男" }
         } );

   }
   cl.insert( docs );
   var cnt = 0;
   do
   {
      ++cnt;
      sleep( 10 );
   }
   while( recordNum != cl.count() && 1000 < cnt );
   assert.equal( recordNum, cl.count() );
}

/******************************************************************************
 * @description: 指定文件名生成文件，并写入数据。会自动删除已经存在的老文件
 * @param {string} fileName  // 文件名
 * @param {string} data      // 数据
 ******************************************************************************/
function generateLobFile ( fileName, data )
{
   removeLobFile( fileName );

   var file = new File( fileName );
   if ( undefined != data )
   {
      file.write( data );
   }
   file.close();
}

/******************************************************************************
 * @description: 删除文件
 * @param {string} fileName  // 文件名
 ******************************************************************************/
function removeLobFile ( fileName ) {
    if ( File.exist( fileName ) ) {
        File.remove( fileName );
    }
}

/******************************************************************************
 * @description: 生成指定长度的 string，可指定字符
 * @param {int} len   // 长度
 * @param {char} char // 字符
 ******************************************************************************/
function generateLobData ( len, char ) {
    if ( undefined == char ) {
        char = 'a';
    }

    var arr = new Array();
    for( var i = 0; i < len; i++ ) {
        arr[i] = char;
    }
    return arr.join('');
 }

 /******************************************************************************
 * @description: 调用 _putLobFile 接口上传 lob 后，并检查 lob 数据是否正确
 * @param {string} cl       // 集合名
 * @param {string} lobId    // lob id，选填
 * @param {string} dataFile // 待上传的 lob 数据文件
 * @param {string} readFile // 读取 lob 时接收数据的文件
 ******************************************************************************/
 function putAndcheckLob( cl, lobId, dataFile, readFile ) {

    if ( undefined == lobId )
    {
        lobId = cl._putLobFile( dataFile );
    }
    else
    {
        cl._putLobFile( dataFile, lobId );
    }

    generateLobFile( readFile );
    cl.getLob( lobId, readFile, true );
    
    var expectMD5 = File.md5( dataFile );
    var actualMD5 = File.md5( readFile );
    removeLobFile( readFile );

    assert.equal( expectMD5, actualMD5 );
 }

 /******************************************************************************
 * @description: 计算 lob 数据大小
 * @param {string} lobNum   // 集合名
 * @param {string} dataFile // 待上传的 lob 数据文件
 ******************************************************************************/
 function getDataSize( lobNum, dataFile ) {
    var dataSize = File.getSize( dataFile ) * lobNum;
    return dataSize;
 }

 /******************************************************************************
 * @description: 检查 cl 快照中的 lob 监控指标。根据 nodeNameArr、clFullName 过滤数据
 * @param {Sdb} db            // coord 节点连接对象
 * @param {Array} nodeNameArr // 数据节点列表
 * @param {string} clFullName // 集合全名
 * @param {int} lobNum        // 集合全名
 * @param {string} dataFile   // 上传的 lob 数据文件
 ******************************************************************************/
 function checkLobInCLSnap( db, nodeNameArr, clFullName, lobNum, dataFile ) {
    var cond = { "Name": clFullName };
    var sel = { "Details.Group.TotalLobs": "",
                "Details.Group.TotalValidLobSize": "",
                "Details.Group.TotalLobPut": "",
                "Details.Group.TotalLobWriteSize": "",
                "Details.Group.NodeName": "" }
    var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, cond, sel );
    var obj = cursor.next().toObj();
    cursor.close();

    var totalLobs = 0;
    var totalValidLobSize = 0;
    var totalLobPut = 0;
    var totalLobWriteSize = 0;

    var detailArr = obj["Details"];
    for ( var i = 0; i < detailArr.length; i++ ) {
        var groupArr = detailArr[i]["Group"];

        for ( var j = 0; j < groupArr.length; j++ ) {
            var info = groupArr[j];

            var nodeName = info["NodeName"];
            if ( nodeNameArr.indexOf( nodeName ) == -1 ) {
                continue;
            }
            totalLobs += info["TotalLobs"];
            totalValidLobSize += info["TotalValidLobSize"];
            totalLobPut += info["TotalLobPut"];
            totalLobWriteSize += info["TotalLobWriteSize"];
        }
    }

    var dataSize = getDataSize( lobNum, dataFile );

    assert.equal( totalLobs, lobNum );
    assert.equal( totalValidLobSize, dataSize );
    assert.equal( totalLobPut, lobNum );
    assert.equal( totalLobWriteSize, dataSize );
 }

 /******************************************************************************
 * @description: 检查当前会话快照中的 lob 监控指标。根据 nodeNameArr 过滤数据
 * @param {Sdb} db            // coord 节点连接对象
 * @param {Array} nodeNameArr // 数据节点列表
 * @param {int} lobNum        // 集合全名
 * @param {string} dataFile   // 上传的 lob 数据文件
 ******************************************************************************/
 function checkLobInCurSessSnap( db, nodeNameArr, lobNum, dataFile ) {
    var cond = { "NodeName": { "$in": nodeNameArr } };
    var sel = { "TotalLobPut": "",
                "TotalLobWriteSize": "",
                "NodeName": "" }
    var cursor = db.snapshot( SDB_SNAP_SESSIONS_CURRENT, cond, sel );

    
    var totalLobPut = 0;
    var totalLobWriteSize = 0;

    var dataSize = getDataSize( lobNum, dataFile );
    try {
        while( cursor.next() ) {
            var info = cursor.current().toObj();
            totalLobPut += info["TotalLobPut"];
            totalLobWriteSize += info["TotalLobWriteSize"];
        }
    } finally {
        cursor.close();
    }

    var dataSize = getDataSize( lobNum, dataFile );

    assert.equal( totalLobPut, lobNum );
    assert.equal( totalLobWriteSize, dataSize );
 }

 /******************************************************************************
 * @description: 获取指定复制组中的主节点 nodeName
 * @param {Sdb} db            // coord 节点连接对象
 * @param {string} groupName  // 复制组名称
 ******************************************************************************/
 function getMasterNodeName( db, groupName ) {
    var rg = db.getRG( groupName );
    var node = rg.getMaster();
    return node.getHostName() + ":" + node.getServiceName();
 }

