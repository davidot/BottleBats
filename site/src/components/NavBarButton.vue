<template>
  <router-link :to="to" v-slot="{ href, navigate, isActive }" custom>
    <div :class="[clazz, 'nav-button', isActive && 'nav-active-link']">
      <a :href="href" @click="navigate">
        <slot />
      </a>
    </div>
  </router-link>
</template>

<script>
export default {
  name: "NavBarButton",
  props: {
    to: {},
    clazz: String,
    enabled: {
      type: Boolean,
      default: true,
    },
  },
};
</script>
