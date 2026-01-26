import { message } from '@/utils/sdb-message'

/**
 * Parse the time to string
 * @param {(Object|string|number)} time
 * @param {string} cFormat
 * @returns {string | null}
 */
export function parseTime(time, cFormat) {
  if (arguments.length === 0 || !time) {
    return null
  }
  const format = cFormat || '{y}-{m}-{d} {h}:{i}:{s}'
  let date
  if (typeof time === 'object') {
    date = time
  } else {
    if ((typeof time === 'string')) {
      if ((/^[0-9]+$/.test(time))) {
        // support "1548221490638"
        time = parseInt(time)
      } else {
        // support safari
        time = time.replace(new RegExp(/-/gm), '/')
      }
    }

    if ((typeof time === 'number') && (time.toString().length === 10)) {
      time = time * 1000
    }
    date = new Date(time)
  }
  const formatObj = {
    y: date.getFullYear(),
    m: date.getMonth() + 1,
    d: date.getDate(),
    h: date.getHours(),
    i: date.getMinutes(),
    s: date.getSeconds(),
    a: date.getDay()
  }
  const time_str = format.replace(/{([ymdhisa])+}/g, (result, key) => {
    const value = formatObj[key]
    // Note: getDay() returns 0 on Sunday
    if (key === 'a') { return ['日', '一', '二', '三', '四', '五', '六'][value ] }
    return value.toString().padStart(2, '0')
  })
  return time_str
}

/**
 * @param {number} time
 * @param {string} option
 * @returns {string}
 */
export function formatTime(time, option) {
  if (('' + time).length === 10) {
    time = parseInt(time) * 1000
  } else {
    time = +time
  }
  const d = new Date(time)
  const now = Date.now()

  const diff = (now - d) / 1000

  if (diff < 30) {
    return '刚刚'
  } else if (diff < 3600) {
    // less 1 hour
    return Math.ceil(diff / 60) + '分钟前'
  } else if (diff < 3600 * 24) {
    return Math.ceil(diff / 3600) + '小时前'
  } else if (diff < 3600 * 24 * 2) {
    return '1天前'
  }
  if (option) {
    return parseTime(time, option)
  } else {
    return (
      d.getMonth() +
      1 +
      '月' +
      d.getDate() +
      '日' +
      d.getHours() +
      '时' +
      d.getMinutes() +
      '分'
    )
  }
}

/**
 * @param {string} url
 * @returns {Object}
 */
export function param2Obj(url) {
  const search = decodeURIComponent(url.split('?')[1]).replace(/\+/g, ' ')
  if (!search) {
    return {}
  }
  const obj = {}
  const searchArr = search.split('&')
  searchArr.forEach(v => {
    const index = v.indexOf('=')
    if (index !== -1) {
      const name = v.substring(0, index)
      const val = v.substring(index + 1, v.length)
      obj[name] = val
    }
  })
  return obj
}

/**
 * @param {number} len
 * @returns {string}
 */
export function randomStr(len) {
  let chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890"
  let res = ''
  let maxPos = chars.length
  for(let i=0; i<len; i++){
    res = res + chars.charAt(Math.floor(Math.random() * maxPos))
  }
  return res
}

/**
 * @param {Map} map
 * @returns {Object}
 */
export function mapToObject(map) {
  let obj = Object.create(null)
  for (let[k,v] of map) {
    obj[k] = v
  }
  return obj
}

/**
 * @param {string|object} data
 * @returns {string}
 */
 export function toPrettyJson(data) {
  if (data instanceof Object) {
    return JSON.stringify(data, null, 2)
  }
  let obj
  try {
    obj = JSON.parse(data)
  }catch(err){
    return data
  }
  return JSON.stringify(obj, null, 2)
}

/**
 * @param {string} str
 * @returns {boolean}
 */
export function isJsonStr (str) {
  if (typeof str == 'string') {
    try {
      let obj = JSON.parse(str)
      if (typeof obj == 'object' && obj) {
        return true
      } else {
        return false
      }
    } catch (e) {
      return false
    }
  }
}

/**
 * @param {object} obj
 */
 export function isObject(obj){
  return Object.prototype.toString.call(obj)==='[object Object]';
};
/**
* @param {Array} arr
*/
export function isArray(arr){
  return Object.prototype.toString.call(arr)==='[object Array]';
};
/**
* @param {object} oldData
* @param {object} newData
*/
 export function equalsObj(oldData, newData){
   if(oldData === newData){
     return true
   }
   if(isObject(oldData) && isObject(newData) && Object.keys(oldData).length === Object.keys(newData).length){
      for (const key in oldData) {
        if (oldData.hasOwnProperty(key)) {
          if(!equalsObj(oldData[key],newData[key])) {
            return false
          }
        }
      }
   }else if(isArray(oldData) && isArray(oldData) && oldData.length===newData.length){
      for (let i = 0,length=oldData.length; i <length; i++) {
        if(!equalsObj(oldData[i],newData[i]))
        return false
      }
   }else{
      return false
   }
   return true
}

/**
 * Convert file size
 * @param {long} fileSize
 * @returns
 */
export function convertFileSize(val) {
  if (val == 0) return "0 B"
  let k = 1024
  let sizes = ["B", "KB", "MB", "GB", "TB", "PB"]
  let i = Math.floor(Math.log(val) / Math.log(k))
  let res = val / Math.pow(k, i)
  res = Math.floor(res * 100) / 100
  return res + " " + sizes[i]
}
/**
 *
 * @param {string} str
 */
export function escapeStr(str) {
  const specialChars = '$^*()+|\\{}[].?'
  str = str + ''
  let newStr = ''
  for (let i=0; i < str.length; i++) {
    let c = str.charAt(i)
    if (specialChars.indexOf(c) != -1) {
      newStr += '\\' + c
    } else {
      newStr += c
    }
  }
  return newStr
}

/**
 *显示批量操作结果
 * @param {array} str
 */
export function showBatchOpMessage(opType, resList) {
  let successList = resList.filter(item => item.success).map(item => item.name)
  let failedList = resList.filter(item => !item.success).map(item => item.name)
  let msg = ""
  if(successList.length > 0) {
    msg = opType + "【" + successList.join(",") + "】成功"
    if (failedList.length > 0) {
      msg += ",【" + failedList.join(",") + "】失败"
    }
  } else {
    msg = opType + "【" + failedList.join(",") + "】失败"
  }
  let hasMessage = false
  for (const r of resList) {
    if (r.message) {
      hasMessage = true
      msg += `<br><br><strong>${r.name}</strong>:${r.message}`
    }
  }
  if (failedList.length == 0) {
    message.success({
      dangerouslyUseHTMLString: true,
      message: msg.replace(/\n/g, '<br>'),
      showClose: hasMessage,
      duration: hasMessage ? 0 : 3000
    })
  } else if (successList.length == 0) {
    message.error({
      dangerouslyUseHTMLString: true,
      message: msg.replace(/\n/g, '<br>'),
      showClose: hasMessage,
      duration: hasMessage ? 0 : 3000
    })
  } else {
    message.warning({
      dangerouslyUseHTMLString: true,
      message: msg.replace(/\n/g, '<br>'),
      showClose: hasMessage,
      duration: hasMessage ? 0 : 3000
    })
  }
}

/**
 * 获取分区字符串
 * @param {string} shardingType
 * @param {string} sep
 * @returns
 */
export function getShardingStr(shardingType, sep = '') {
  const date = new Date()
  const year = date.getFullYear()
  const month = date.getMonth() + 1
  const day = date.getDate()

  if (shardingType === 'year') {
    return sep + year
  } else if (shardingType === 'month') {
    return sep + year + month.toString().padStart(2, '0')
  } else if (shardingType === 'day') {
    return sep + year + month.toString().padStart(2, '0') + day.toString().padStart(2, '0')
  } else if (shardingType === 'quarter') {
    const quarter = Math.floor((month - 1) / 3) + 1
    return sep + year + 'Q' + quarter
  }
  return ''
}

/**
 * 获取指定时间戳当天的最大时间
 * @param {number} timestamp
 * @returns
 */
export function getMaxTimestampForDay(timestamp) {
  const date = new Date(timestamp)
  date.setHours(23, 59, 59, 999)
  return date.getTime()
}


/**
 * 格式化字节数，转化为 GB 或 MB
 * @param {*} bytes
 * @returns
 */
export function formatBytes(bytes) {
  const units = ["B", "KB", "M", "G"]
  let unitIndex = 0
  let size = Math.abs(bytes)
  let isNegative = bytes < 0

  while (size >= 1024 && unitIndex < units.length - 1) {
      size /= 1024
      unitIndex++
  }

  size = parseFloat(size.toFixed(2))

  if(isNegative) {
    size = -size
  }

  return {
      value: size,
      unit: units[unitIndex],
      text: size + units[unitIndex]
  }
}



