<template>
  <div>
    <p>Bots</p>
    <div v-if="!loggedIn">
      Sorry, your not logged in so not much to see here.
    </div>
    <BotCreator v-else @new-bot="console.log('new bot added')" :upload-url="'/elevated/upload'"/>
    <div style="display: flex; flex-direction: column;">
      <div v-if="bots === null"><div style="animation: 1s linear infinite; transform-origin: 50% 50%;">Loading</div></div>
      <div :bot="bot" v-for="bot in bots" :key="'bot' + bot.name" />
    </div>
  </div>
</template>

<script>
import BotCreator from "@/components/BotCreator.vue";
import {endpoint} from "@/http";

export default {
  name: "BotView",
  components: {
    BotCreator,
  },
  inject: ["userDetails"],
  mounted() {
    this.botUpdateInterval = setInterval(() => this.updateBots(), 15000);
    this.updateBots();
  },
  unmounted() {
    if (this.botUpdateInterval)
      clearInterval(this.botUpdateInterval);
  },
  data() {
    return {
      botUpdateInterval: null,
      bots: null,
      connectionLost: false,
    };
  },
  computed: {
    loggedIn() {
      return this.userDetails.values.value.displayName != null;
    },
  },
  methods: {
    async updateBots() {
      try {
        const data = await endpoint.get("/elevated/bots", {timeout: 750});
        this.bots = data.data;
        this.connectionLost = false;
      } catch {
        this.connectionLost = true;
      }
    },
  },
};
</script>

<style scoped>

</style>
