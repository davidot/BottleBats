<template>
  <main>
    <RouterView />
    <VijfGame :start="start" :moves="moves" />
  </main>
</template>

<script setup lang="ts">
import { RouterView } from "vue-router";
import VijfGame from "@/components/games/VijfGame.vue";
import {onMounted, ref} from "vue";

const start = ref("");
const moves = ref("");

onMounted(() => {
  setTimeout(() => {
    start.value = "AJ4;T98;AK6;KQ7;942;345*259T26Q5*87A754AT83J96KQ38T6KJQ37J+2;";
    moves.value = "10 A8A723*426";
  }, 3000);
});
</script>

<style>

</style>
