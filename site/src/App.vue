<template>
  <main>
    <RouterView />
    <VijfGame />
  </main>
</template>

<script setup lang="ts">
import { RouterView } from "vue-router";
import LeaderBoard from "@/components/Leaderboard.vue";
import VijfGame from "@/components/games/VijfGame.vue";
</script>

<style>

</style>
