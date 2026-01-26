import request from '@/utils/request'

const BASE_API = '/api/v1'



 export function getTaskProgress(taskId) {
  return request({
    url: BASE_API+'/tasks/' + taskId + '/progress',
    method: 'get'
  })
}

export function stopTask(taskId) {
  return request({
    url: BASE_API+'/tasks/' + taskId + '/stop',
    method: 'post'
  })
}

