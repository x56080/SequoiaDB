import Vue from 'vue'
import Router from 'vue-router'

Vue.use(Router)

/* Layout */
import Layout from '@/layout'

/**
 * Note: sub-menu only appear when route children.length >= 1
 * Detail see: https://panjiachen.github.io/vue-element-admin-site/guide/essentials/router-and-nav.html
 *
 * hidden: true                   if set true, item will not show in the sidebar(default is false)
 * alwaysShow: true               if set true, will always show the root menu
 *                                if not set alwaysShow, when item has more than one children route,
 *                                it will becomes nested mode, otherwise not show the root menu
 * redirect: noRedirect           if set noRedirect will no redirect in the breadcrumb
 * name:'router-name'             the name is used by <keep-alive> (must set!!!)
 * meta : {
    roles: ['admin','editor']    control the page roles (you can set multiple roles)
    title: 'title'               the name show in sidebar and breadcrumb (recommend set)
    icon: 'svg-name'/'el-icon-x' the icon show in the sidebar
    breadcrumb: false            if set false, the item will hidden in breadcrumb(default is true)
    activeMenu: '/example/list'  if set path, the sidebar will highlight the path you set
  }
 */

/**
 * constantRoutes
 * a base page that does not have permission requirements
 * all roles can be accessed
 */
export const constantRoutes = [
  {
    path: '/',
    redirect: '/schedule/table'  // <--- 这里设置默认跳转
  },

  {
    path: '/404',
    component: () => import('@/views/404'),
    hidden: true
  },

  {
    path: '/node',
    component: Layout,
    meta: { title: '节点管理', icon: 'el-icon-monitor' },
    redirect: '/site/table',
    children: [
      {
        path: 'table',
        name: 'Table',
        component: () => import('@/views/node/index'),
        meta: { title: '节点管理', icon: 'el-icon-monitor', keepAlive:true }
      },
    ]
  },

  
  {
    path: '/site',
    component: Layout,
    meta: { title: '站点管理', icon: 'el-icon-s-help' },
    redirect: '/site/table',
    children: [
      {
        path: 'table',
        name: 'Table',
        component: () => import('@/views/site/index'),
        meta: { title: '站点管理', icon: 'el-icon-s-help', keepAlive:true }
      },
    ]
  },

  {
    path: '/schedule',
    component: Layout,
    meta: { title: '调度任务', icon: 'el-icon-tickets' },
    redirect: '/schedule/table',
    children: [
      {
        path: 'table',
        name: 'Table',
        component: () => import('@/views/schedule/index'),
        meta: { title: '调度任务管理', icon: 'el-icon-tickets', keepAlive:true }
      },
      {
        path: '/schedule/runRecord',
        name: 'RunRecord',
        hidden: true,
        component: () => import('@/views/schedule/components/runRecord.vue'),
        meta: { title: '运行记录', noCache: true }
      }
    ]
  },

  {
    path: '/globalconf',
    component: Layout,
    meta: { title: '系统配置管理', icon: 'el-icon-s-tools' },
    redirect: '/globalconf/table',
    children: [
      {
        path: 'table',
        name: 'Table',
        component: () => import('@/views/globalconf/index'),
        meta: { title: '系统配置管理', icon: 'el-icon-s-tools', keepAlive:true }
      },
    ]
  },


  // 404 page must be placed at the end !!!
  { path: '*', redirect: '/404', hidden: true }
]

const createRouter = () => new Router({
  // mode: 'history', // require service support
  scrollBehavior: () => ({ y: 0 }),
  routes: constantRoutes
})

const router = createRouter()

// Detail see: https://github.com/vuejs/vue-router/issues/1234#issuecomment-357941465
export function resetRouter() {
  const newRouter = createRouter()
  router.matcher = newRouter.matcher // reset router
}

export default router
