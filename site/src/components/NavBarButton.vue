<template>
  <router-link :to="toWithGame" v-slot="{ href, navigate, isActive }" custom>
    <div :class="[clazz, 'nav-button', isActive && 'nav-active-link', (!enabled) && 'disabled']" @click="navigate">
      <a :href="href">
        <slot />
      </a>
    </div>
  </router-link>
</template>

<script>

import { defaultGame } from '@/router';

export default {
  name: "NavBarButton",
  props: {
    to: String,
    clazz: String,
    enabled: {
      type: Boolean,
      default: true,
    },
  },
  computed: {
    toWithGame() {
      const route = this.$router.options.routes.find(r => r.name === this.to);
      if (!route.path.includes(':game')) {
        return {
          name: this.to
        };
      }
      
      return {
        name: this.to,
        params: {
          game: this.$route.params.game || defaultGame,
        }
      };
    }
  }
};
</script>
