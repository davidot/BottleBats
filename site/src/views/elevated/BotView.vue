<template>
  <div>
    <p>Bots</p>
    <div v-if="!loggedIn">
      Sorry, your not logged in so not much to see here.
    </div>
    <button @click="updateBots">Add bot</button>
    <BotCreator @new-bot="updateBots" :upload-url="'/elevated/upload'"/>
    <div style="display: flex; flex-direction: column; align-items: center">
      <div v-if="bots === null">Loading</div>
      <BotInfo v-for="bot in bots" :key="'bot' + bot.name" :bot="bot" />
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
    // this.botUpdateInterval = setInterval(() => this.updateBots(), 15000);
    // this.updateBots();

    setTimeout(() => {
      this.bots = [
        {id: 2, name: 'bot 2', running: true, status: 'false'},
        {id: 3, name: 'bot 3', running: false, status: 'disabled'},
      ];
    }, 500);


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
      // setTimeout(() => {
      //   this.bots = [
      //     {id: 2, name: 'bot 2', running: true, status: 'false'},
      //     {id: 3, name: 'bot 3', running: false, status: 'disabled'},
      //     {id: 4, name: 'bot 4', running: false, status: 'Checking and building file'},
      //   ];
      // }, 500);
      //
      // setTimeout(() => {
      //   this.bots = [
      //     {id: 2, name: 'bot 2', running: true, status: 'false'},
      //     {id: 3, name: 'bot 3', running: false, status: 'disabled'},
      //     {id: 4, name: 'bot 4', running: true, status: 'Running cases'},
      //   ];
      // }, 1500);
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
