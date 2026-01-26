import request from '@/utils/request'

const BASE_API = '/api/v1'

/**
 * 查询系统配置列表
 * @returns
 */
export function queryGlobalConfList(page, size, filter) {
  return request({
    url: BASE_API+'/global_conf',
    method: 'get',
    params: {
      skip: size ? (page-1)*size : 0,
      limit: size ? size : -1,
      filter: filter
    }
  })
}

export function updateConf(conf_key, conf_value) {
  return request({
    url: BASE_API+'/global_conf',
    method: 'put',
    params: {
      conf_key: conf_key,
      conf_value: conf_value
    }
  })
}