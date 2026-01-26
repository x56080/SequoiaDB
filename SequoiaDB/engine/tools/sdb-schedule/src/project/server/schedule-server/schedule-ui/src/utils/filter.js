import Vue  from 'vue'
import {convertFileSize, parseTime} from '@/utils/index'


Vue.filter('parseTime',(time, format)=>{
  return parseTime(time, format)
})

Vue.filter('arrayJoin',(arr, separate)=>{
  if(arr && arr instanceof Array){
    if(!separate){
      separate =  ","
    }
    return arr.join(separate)
  }
  return arr
})

Vue.filter('convertFileSize', (val)=>{
  return convertFileSize(val)
})