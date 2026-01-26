import request from '@/utils/request'

const BASE_API = '/api/v1'

/**
 * 查询站点列表
 * @returns
 */
export function querySiteList(page, size, filter) {
  return request({
    url: BASE_API+'/sites',
    method: 'get',
    params: {
      skip: size ? (page-1)*size : 0,
      limit: size ? size : -1,
      filter: filter
    }
  })
}