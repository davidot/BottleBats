<template>
  <main>
    <NavBar />
    <div class="main-content">
      <RouterView />
<!--      <VijfGame :start="start" :moves="moves" />-->
<!--      <LogoSVG style="width: 400px"/>-->
    </div>
  </main>
</template>

<script setup lang="ts">
import { RouterView } from "vue-router";
import VijfGame from "@/components/vijf/VijfGame.vue";
import NavBar from "@/components/NavBar.vue";
import {onMounted, provide, ref} from "vue";
import LogoSVG from "@/components/LogoSVG.vue";
import {endpoint} from "./http";

const start = ref("");
const moves = ref("");

// onMounted(() => {
//   setTimeout(() => {
    start.value = "AJ4;T98;AK6;KQ7;942;345*259T26Q5*87A754AT83J96KQ38T6KJQ37J+2;";
    moves.value = "10 A8A723*426";
  // }, 1000);
// });

const userDetails = ref({
  displayName: null,
  isAdmin: false
});

function updateUserDetails() {
  endpoint.get("/auth/info").then((val) => {
    console.log('Auth successful!');
      const userData = val.data;
      userDetails.value.displayName = userData.displayName;
      userDetails.value.isAdmin = userData.admin;
  }).catch((err) => {
      console.log('Not logged in (anymore)', err);
    userDetails.value.displayName = null;
    userDetails.value.isAdmin = false;
  });
}

provide("userDetails", {
  values: userDetails,
  updateUserDetails,
});

updateUserDetails();

</script>

<style>

body {
  margin-top: 0;
}

main {
  width: calc(max(1200px, 60%));
  margin: auto;
}

.bb-nav {
  position: sticky;
  top: 0;
  height: 50px;
  width: 100%;
  z-index: 100;
  background: #444cf7;
  text-align: center;
  /*vertical-align: center;*/
}

.main-content {
  position: relative;
  top: 50px;
    /*padding-top: 5px;*/
}

</style>
