import defaultSettings from '@/settings'

const title = defaultSettings.title || 'Sdb-Schedule'

export default function getPageTitle(pageTitle) {
  if (pageTitle) {
    return `${pageTitle} - ${title}`
  }
  return `${title}`
}
