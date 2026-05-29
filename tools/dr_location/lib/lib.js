// 获取当前时间戳，忽略微秒
function getCurrentTimestamp() {
    var now = new Date();
    var year = now.getFullYear();
    var month = now.getMonth() + 1;
    var day = now.getDate();
    var hour = now.getHours();
    var minute = now.getMinutes();
    var second = now.getSeconds();

    var pad = function(num) {
        return String(num).length < 2 ? '0' + String(num) : String(num);
    };
    return year + "-" +
           pad(month) + "-" +
           pad(day) + "-" +
           pad(hour) + "." +
           pad(minute) + "." +
           pad(second) + ".000000";
}

// 计算两个时间的差值，忽略毫秒，返回秒数（可能为负）
function getTimeDiff(time1, time2) {
    var array = time1.split('.');
    var timeStr1 = (array[0] + '.' + array[1] + '.' + array[2]).replace(/^(\d{4}-\d{2}-\d{2})-/,'$1T').replace(/\./g,':');
    var t1 = new Date(timeStr1);

    array = time2.split('.');
    var timeStr2 = (array[0] + '.' + array[1] + '.' + array[2]).replace(/^(\d{4}-\d{2}-\d{2})-/,'$1T').replace(/\./g,':');
    var t2 = new Date(timeStr2);

    return (t1 - t2) / 1000;
}

// 重复字符串辅助函数
var repeatStr = function(str, count) {
    var result = "";
    for (var i = 0; i < count; i++) {
        result += str;
    }
    return result;
};

// 常量：重复的等号，用于分隔线
var separator = repeatStr("=", 20);

// 根据下标和值创建数组
function createArrayByIndex(index, content) {
    var array = [];
    for (let i = 0; i < index; i++) {
        array.push("");
    }
    array.push(content);
    return array;
}

/*
    打印表格到文件，会自动补齐表格数量，自适应字符串长度
    入参：
        outputArray, 打印内容，格式如下：
            var outputArray = [
                {
                    "head": [
                        [ "","a","b","c" ],
                        [ "","d","e","f" ],
                    ],
                    "content": [
                        [ "1","2","3","4" ],
                        [ "5","6","7","8" ]
                    ],
                },
                {
                    "head": [
                        [ "","a","b","c" ],
                        [ "","d","e","f" ],
                    ],
                    "content": [
                        [ "1","2","1234","4" ],
                        [ "5","6","7","8" ]
                    ],
                }
            ];
            打印效果：
                |------|------|------|------|
                |      | a    | b    | c    |
                |      | d    | e    | f    |
                |------|------|------|------|
                | 1    | 2    | 3    | 4    |
                | 5    | 6    | 7    | 8    |
                |------|------|------|------|

                |------|------|------|------|
                |      | a    | b    | c    |
                |      | d    | e    | f    |
                |------|------|------|------|
                | 1    | 2    | 1234 | 4    |
                | 5    | 6    | 7    | 8    |
                |------|------|------|------|
        fileName, 写入的文件名
    返回值
        正确：无
        错误：throw error
*/
function outputSheet(outputArray, fileName) {
    if (0 == outputArray.length) {
        return true;
    }

    // 打开文件
    let file;
    if (null == fileName || "" == fileName) {
        throw new Error("[ERROR] fileName can not be empty");
    }
    try {
        if (File.exist(fileName)) {
            File.remove(fileName);
        }
    } catch (error) {
        throw new Error("[ERROR] Failed to clean [ " + fileName + " ], error info: " + getLastErrObj());
    }

    // 检查并创建上层目录
    let index = fileName.lastIndexOf('/');
    let dir = fileName.substring(0, index);

    try {
        if (!File.exist(dir)) {
            File.mkdir(dir, 0755);
        }
    } catch(error) {
        throw new Error("[ERROR] Failed to create dir [ " + dir + " ], error info: " + getLastErrObj());
    }

    try {
        file = new File(fileName, 0644);
    } catch(error) {
        throw new Error("[ERROR] Failed to create or open file [ " + fileName + " ], error info: " + getLastErrObj());
    }

    // 循环打印
    try {
        for (let i = 0; i < outputArray.length; i++) {
            // 计算当前表格字符串最大长度
            let maxLength = 0;
            // 表格最大数量
            let chartLength = 0;
            let headArray = outputArray[i].head;
            for (let j = 0; j < headArray.length; j++) {
                chartLength = chartLength > headArray[j].length ? chartLength : headArray[j].length;
                for (let k = 0; k < headArray[j].length; k++) {
                    if (headArray[j][k]) {
                        maxLength = maxLength > headArray[j][k].length ? maxLength : headArray[j][k].length;
                    }
                }
            }
            let contentArray = outputArray[i].content;
            for (let j = 0; j < contentArray.length; j++) {
                chartLength = chartLength > contentArray[j].length ? chartLength : contentArray[j].length;
                for (let k = 0; k < contentArray[j].length; k++) {
                    if (contentArray[j][k]) {
                        maxLength = maxLength > contentArray[j][k].length ? maxLength : contentArray[j][k].length;
                    }
                }
            }

            // 构造出最大长度的 |-------|-------| 用做分隔横线
            var line = "|";
            // 需要给字符串留出左右两个空格，所以长度需要加 2
            for (let i = 0; i < maxLength + 2; i++) {
                line += "-";
            }
            let tmp = "";
            // 按照表格长度拼接
            for (let i = 0; i < chartLength; i++) {
                tmp += line;
            }
            line = tmp + "|";

            // 打印 title
            if (null != outputArray[i].title) {
                file.write(outputArray[i].title + '\n');
            }

            // 打印分隔横线
            file.write(line + '\n');

            // head 行
            let headDoubleArray = outputArray[i].head;
            for (let j = 0; j < headDoubleArray.length; j++) {
                let headArray = headDoubleArray[j];
                for (let k = 0; k < headArray.length; k++) {
                    let space = "";
                    let head = headArray[k] ? headArray[k] : "";
                    // 补齐空格，需要给右边留出一个空格，所以使用 <= 多打印一个
                    for (let m = 0; m <= maxLength - head.length; m++) {
                        space += " ";
                    }
                    // 打印内容，左右都多一个空格
                    file.write("| " + head + space);
                }
                // 补齐空表格
                for (let k = 0; k < chartLength - headArray.length; k++) {
                    let space = "";
                    for (let m = 0; m <= maxLength; m++) {
                        space += " ";
                    }
                    // 打印内容，左右都多一个空格
                    file.write("| " + space);
                }
                // 结束换行
                file.write("|" + '\n');
            }

            // 打印分隔横线
            file.write(line + '\n');

            // content 行
            let contentDoubleArray = outputArray[i].content;
            for (let j = 0; j < contentDoubleArray.length; j++) {
                let contentArray = contentDoubleArray[j];
                for (let k = 0; k < contentArray.length; k++) {
                    let space = "";
                    let content = contentArray[k] ? contentArray[k] : "";
                    // 补齐空格，需要给右边留出一个空格，所以使用 <= 多打印一个
                    for (let m = 0; m <= maxLength - content.length; m++) {
                        space += " ";
                    }
                    // 打印内容，左右都多一个空格
                    file.write("| " + content + space);
                }
                // 补齐空表格
                for (let k = 0; k < chartLength - contentArray.length; k++) {
                    let space = "";
                    for (let m = 0; m <= maxLength; m++) {
                        space += " ";
                    }
                    // 打印内容，左右都多一个空格
                    file.write("| " + space);
                }
                // 结束换行
                file.write("|" + '\n');
            }

            // 打印分隔横线
            file.write(line + '\n');

            // 一个表格结束，打印空行
            if (i < outputArray.length - 1) {
                file.write('\n');
            }

        }
    } catch (error) {
        throw new Error('[ERROR] Failed to output chart, error info: ' + error + '(' + getLastErrObj() + ')');
    }
}

// 格式化持续时间
function formatDuration(minutes) {
    if (minutes === null || minutes === undefined) {
        return "";
    }
    var hours = Math.floor(minutes / 60);
    var mins = minutes % 60;
    var pad = function(num) {
        return String(num).length < 2 ? '0' + String(num) : String(num);
    };
    return pad(hours) + ":" + pad(mins);
}

// 格式化时间戳为秒级
function formatTimestampToSeconds(timestamp) {
    if (typeof timestamp === "string") {
        var parts = timestamp.split("-");
        if (parts.length >= 3) {
            var pad = function(num) {
                return String(num).length < 2 ? '0' + String(num) : String(num);
            };
            return parts[0] + "-" +
                   pad(parts[1]) + "-" +
                   pad(parts[2]) + " " +
                   pad(parts[3]) + ":" +
                   pad(parts[4]) + ":" +
                   pad(parts[5]);
        }
    }
    return timestamp;
}

// 打印表格标题
function printTableHeader(headers, rowLength) {
    var headerLine = "|";
    for (var i = 0; i < headers.length; i++) {
        var header = headers[i];
        var padding = Math.floor((rowLength - header.length) / 2);
        headerLine += " " + " ".repeat(padding) + header + " ".repeat(padding + 1);
    }
    headerLine += "|";
    print(headerLine);

    var separatorLine = "|";
    for (var i = 0; i < headers.length; i++) {
        separatorLine += " " + "-".repeat(rowLength) + " ";
    }
    separatorLine += "|";
    print(separatorLine);
}

// 打印表格行
function printTableRow(row, rowLength) {
    var line = "|";
    for (var i = 0; i < row.length; i++) {
        var cell = row[i];
        var padding = Math.floor((rowLength - cell.length) / 2);
        line += " " + " ".repeat(padding) + cell + " ".repeat(padding + 1);
    }
    line += "|";
    print(line);
}

// 打印 Location 信息表格
function printLocationInfo(locationInfo, groupModeInfo) {
    var locations = [];
    for (var i = 0; i < locationInfo.length; i++) {
        locations.push(locationInfo[i].LocationName);
    }
    locations.sort();

    if (locations.length === 0) {
        print("\nNo locations configured.");
        return;
    }

    var maxLocNameLen = 0;
    for (var i = 0; i < locations.length; i++) {
        if (locations[i].length > maxLocNameLen) {
            maxLocNameLen = locations[i].length;
        }
    }
    var rowLength = maxLocNameLen + 8;

    printTableHeader(["Location", "Status"], rowLength);

    for (var i = 0; i < locations.length; i++) {
        var locName = locations[i];
        var isActive = false;
        var groupStatus = "";

        // 使用游标方式查找 Location 信息
        var cursor = new org.bson.BasicDBObject();
        cursor.put("LocationName", locName);
        var locCursor = locationInfo.find(cursor);
        while (locCursor.hasNext()) {
            var locData = locCursor.next();
            isActive = (locData.ActiveStatus === "All");
            groupStatus = locData.GroupStatus;
        }
        locCursor.close();

        var statusStr = locName;
        if (isActive) {
            statusStr += "(active)";
        }

        var modeStr = "";
        if (groupStatus !== "") {
            modeStr = "[" + groupStatus + "]";
        }

        printTableRow([statusStr, modeStr], rowLength);
    }
}

// 打印 GroupMode 信息表格
function printGroupModeInfo(groupModeInfo, maxKeepTime) {
    if (groupModeInfo.length === 0) {
        print("\nNo GroupMode information available.");
        return;
    }

    var maxGroupNameLen = 0;
    for (var i = 0; i < groupModeInfo.length; i++) {
        var nameLen = groupModeInfo[i].GroupName.length;
        if (nameLen > maxGroupNameLen) {
            maxGroupNameLen = nameLen;
        }
    }

    var headers = ["Group", "Mode", "Start Time", "Duration (min)", "Remaining (min)", "Max End Time"];
    var rowLength = maxGroupNameLen + 25;

    printTableHeader(headers, rowLength);

    for (var i = 0; i < groupModeInfo.length; i++) {
        var modeInfo = groupModeInfo[i];
        var mode = modeInfo.GroupMode || "";
        var startTime = modeInfo.UpdateTime || "";
        var duration = calculateDurationMinutes(startTime);
        var remaining = calculateRemainingMinutes(startTime, maxKeepTime);
        var maxEndTime = startTime;
        if (maxKeepTime !== null && maxKeepTime !== undefined) {
            var maxEnd = new Date(formatTimestampToSeconds(startTime));
            maxEnd.setMinutes(maxEnd.getMinutes() + maxKeepTime);
            maxEndTime = formatTimestampToSeconds(maxEnd);
        }

        printTableRow([
            modeInfo.GroupName,
            mode,
            formatTimestampToSeconds(startTime),
            duration,
            remaining,
            formatTimestampToSeconds(maxEndTime)
        ], rowLength);
    }
}

// 计算持续时间（分钟）
function calculateDurationMinutes(startTime) {
    if (!startTime) {
        return null;
    }
    var start = new Date(formatTimestampToSeconds(startTime));
    var now = new Date();
    var diffMs = now - start;
    var diffMins = Math.floor(diffMs / (1000 * 60));
    return diffMins;
}

// 计算剩余时间（分钟）
function calculateRemainingMinutes(startTime, maxKeepTime) {
    if (!startTime || maxKeepTime === null || maxKeepTime === undefined) {
        return null;
    }
    var start = new Date(formatTimestampToSeconds(startTime));
    var end = new Date(start.getTime() + maxKeepTime * 60 * 1000);
    var now = new Date();
    var diffMs = end - now;
    var diffMins = Math.floor(diffMs / (1000 * 60));
    return diffMins;
}

// 打印异常信息
function printExceptionInfo(exceptionInfo) {
    if (!exceptionInfo) {
        return;
    }

    // 打印异常主机
    if (exceptionInfo.NoLocationHost && exceptionInfo.NoLocationHost.length > 0) {
        print("\n[ExceptionHostInfo] No Location Hosts:");
        var headers = ["Host", "Issue"];
        var rowLength = 40;
        printTableHeader(headers, rowLength);
        for (var i = 0; i < exceptionInfo.NoLocationHost.length; i++) {
            printTableRow([exceptionInfo.NoLocationHost[i], "No location configured"], rowLength);
        }
    }

    if (exceptionInfo.PartialLocationHost && exceptionInfo.PartialLocationHost.length > 0) {
        print("\n[ExceptionHostInfo] Partial Location Hosts:");
        var headers = ["Host", "Issue"];
        var rowLength = 40;
        printTableHeader(headers, rowLength);
        for (var i = 0; i < exceptionInfo.PartialLocationHost.length; i++) {
            printTableRow([exceptionInfo.PartialLocationHost[i], "Mixed location nodes"], rowLength);
        }
    }

    if (exceptionInfo.MultiLocationHost && exceptionInfo.MultiLocationHost.length > 0) {
        print("\n[ExceptionHostInfo] Multi Location Hosts:");
        var headers = ["Host", "Issue"];
        var rowLength = 40;
        printTableHeader(headers, rowLength);
        for (var i = 0; i < exceptionInfo.MultiLocationHost.length; i++) {
            printTableRow([exceptionInfo.MultiLocationHost[i], "Nodes in multiple locations"], rowLength);
        }
    }

    // 打印异常组
    if (exceptionInfo.NoLocationGroup && exceptionInfo.NoLocationGroup.length > 0) {
        print("\n[ExceptionGroupInfo] No Location Groups:");
        var headers = ["Group", "Issue"];
        var rowLength = 40;
        printTableHeader(headers, rowLength);
        for (var i = 0; i < exceptionInfo.NoLocationGroup.length; i++) {
            printTableRow([exceptionInfo.NoLocationGroup[i], "No location configured"], rowLength);
        }
    }

    if (exceptionInfo.PartialLocationGroup && exceptionInfo.PartialLocationGroup.length > 0) {
        print("\n[ExceptionGroupInfo] Partial Location Groups:");
        var headers = ["Group", "Issue"];
        var rowLength = 40;
        printTableHeader(headers, rowLength);
        for (var i = 0; i < exceptionInfo.PartialLocationGroup.length; i++) {
            printTableRow([exceptionInfo.PartialLocationGroup[i], "Mixed location nodes"], rowLength);
        }
    }

    if (exceptionInfo.OneLocationGroup && exceptionInfo.OneLocationGroup.length > 0) {
        print("\n[ExceptionGroupInfo] One Location Groups:");
        var headers = ["Group", "Issue"];
        var rowLength = 40;
        printTableHeader(headers, rowLength);
        for (var i = 0; i < exceptionInfo.OneLocationGroup.length; i++) {
            printTableRow([exceptionInfo.OneLocationGroup[i], "Single location configured"], rowLength);
        }
    }
}
