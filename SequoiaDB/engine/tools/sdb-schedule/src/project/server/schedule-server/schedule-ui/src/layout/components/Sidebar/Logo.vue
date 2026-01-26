<template>
  <div class="sidebar-logo-container" :class="{'collapse':collapse}">
    <transition name="sidebarLogoFade">
      <router-link v-if="collapse" key="collapse" class="sidebar-logo-link" to="/">
        <img v-if="logo" :src="logo" class="sidebar-logo-collapse">
        <h1 v-else class="sidebar-title"><span style="color: rgb(235,97,0)">SCM</span></h1>
      </router-link>
      <router-link v-else key="expand" class="sidebar-logo-link" to="/">
        <img v-if="logo" :src="logo" class="sidebar-logo">
        <h1 class="sidebar-title">Sequoia<span style="color: rgb(235,97,0)">DB</span></h1>
      </router-link>
    </transition>
  </div>
</template>

<script>
export default {
  name: 'SidebarLogo',
  props: {
    collapse: {
      type: Boolean,
      required: true
    }
  },
  data() {
    return {
      logo: require('@/assets/logo.png')
    }
  }
}
</script>

<style lang="scss" scoped>
.sidebarLogoFade-enter-active {
  transition: opacity 1.5s;
}

.sidebarLogoFade-enter,
.sidebarLogoFade-leave-to {
  opacity: 0;
}

.sidebar-logo-container {
  position: relative;
  width: 100%;
  height: 50px;
  line-height: 50px;
  text-align: left;
  padding-left: 10px;
  overflow: hidden;

  & .sidebar-logo-link {
    height: 100%;
    width: 100%;

    & .sidebar-logo-collapse {
      width: 70%;
      height: 30px;
      padding-bottom: 5px;
      vertical-align: bottom;
    }

    & .sidebar-logo {
      width: 15%;
      height: 35px;
      padding-bottom: 5px;
      vertical-align: bottom;
    }

    & .sidebar-title {
      display: inline-block;
      padding-bottom: 5px;
      margin: 0;
      color: #fff;
      font-weight: 600;
      line-height: 30px;
      font-size: 15px;
      text-align: left;
      text-indent: 5px;
      font-weight: bold;
      font-family: Avenir, Helvetica Neue, Arial, Helvetica, sans-serif;
      vertical-align: bottom;
    }
  }

  &.collapse {
    .sidebar-logo {
      margin-right: 0px;
    }
  }
}
</style>
