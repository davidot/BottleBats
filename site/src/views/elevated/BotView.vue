<template>
  <div style="position: relative">
    <div class="disconnected" :style="{top: connectionLost ? '0' : '-100%', 'flex-direction': (isRuben && connectionLost) ? 'column' : 'row'}">
      <img src="/siren1.gif" style="height: 1.5em" alt="siren which means things are going wrong"/>
      <span style="border: 1px solid red; padding: 2px 5px; font-weight: bold">
        Connection to server lost!
      </span>
      <img src="/siren1.gif" style="height: 1.5em" alt="siren which means things are going wrong"/>
    </div>
    <p>Bots</p>
    <div v-if="!loggedIn">
      Sorry, your not logged in so not much to see here.
    </div>
    <BotCreator @new-bot="updateBots" :upload-url="'/elevated/upload'" :multiple="true" :with-image="true" ref="creator"/>
    <div style="display: flex; flex-direction: column; align-items: center">
      <div v-if="bots === null">Loading</div>
      <BotInfo v-for="bot in bots" :key="'bot' + bot.id" :bot="bot" @build-update="updateBots" @image-copy="$refs.creator.useImage" />
    </div>
  </div>
</template>

<script>
import BotCreator from "@/components/BotCreator.vue";
import {endpoint} from "@/http";
import BotInfo from "@/components/elevated/BotInfo.vue";

export default {
  name: "BotView",
  components: {
    BotInfo,
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
    isRuben() {
      return this.userDetails.values.value.isRuben;
    },
  },
  methods: {
    async updateBots() {
      try {
        const data = await endpoint.get("/elevated/my-bots", {timeout: 750});
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
