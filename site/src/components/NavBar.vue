<template>
  <div class="bb-nav">
    <div class="open-doors" style="height: 100%; float: left; z-index: 1; border: 0">
      <div style="width: 100%; height: 100%; background-color: #ebffcf; animation: behind-doors step-end 1.5s forwards">
      </div>
      <LogoSVG class="logo-nav" style="height: 100%; z-index: 1"/>
    </div>
    <span class="nav-button logo-nav" style="border-left-width: 0;">
      Bottle Bats
    </span>

    <NavBarButton :to="{name: 'home'}">
      LeaderBoard
    </NavBarButton>
<!--    <NavBarButton :to="{name: 'games'}">-->
<!--      Games-->
<!--    </NavBarButton>-->
    <NavBarButton :to="{name: 'bots'}" :enabled="loggedIn">
      Bots
    </NavBarButton>
    <NavBarButton :to="{name: 'rules'}">
      Regels
    </NavBarButton>
    <NavBarButton :to="{path: '/vijf'}">
      Vijf
    </NavBarButton>

    <NavBarButton :to="{name: 'login'}" :clazz="'login-side'" v-if="!loggedIn">
      Log in / Register
    </NavBarButton>
    <span v-else class="nav-button login-side" style="border-right: 0;" @click="logOut">
      <a>
        Logout
      </a>
    </span>
    <div v-if="loggedIn" class="nav-button login-side nav-active-link" style="cursor: default">
      Welcome, {{ username }}
    </div>
  </div>
</template>

<script>
import NavBarButton from "@/components/NavBarButton.vue";
import LogoSVG from "@/components/LogoSVG.vue";
import {endpoint} from "@/http";
export default {
  name: "TopBar",
  components: {LogoSVG, NavBarButton},
  inject: ["userDetails"],
  computed: {
    loggedIn() {
      return this.userDetails.values.value.displayName != null;
    },
    username() {
      if (this.loggedIn) return this.userDetails.values.value.displayName;
      return "Not logged in";
    },
  },
  methods: {
    logOut() {
      endpoint
          .post("/auth/logout", '')
          .then(() => {
            this.userDetails.updateUserDetails();
            console.log('Log out sucessful!');
          })
          .catch((err) => {
            console.log('Log out failed? probably just server disconnect?', err);
            this.userDetails.updateUserDetails();
          });
    }
  }
};
</script>

<style>

.nav-button {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 0 10px;
  float: left;

  height: 100%;
  background: #5156bf;
  border: 1px solid white;
  border-top: 0;
  border-bottom: 0;
  cursor: pointer;
}

.nav-button:hover {
  background-color: #6165f5;
}

.nav-button.nav-active-link {
  background-color: #b6b8f5;
}

.nav-button a {
  text-decoration: none;
  color: ghostwhite;
}

.nav-button.nav-active-link a {
  color: black;
}

.login-side {
  float: right;
}

.nav-button.disabled {
  background-color: gray;
  text-decoration: line-through;
  pointer-events: none;
}

.logo-nav {
  background-color: #ebffcf;
  cursor: default;
}

.logo-nav:hover {
  background-color: #ebffcf;
}


.open-doors {
  animation: behind-doors;
  position: relative;
  overflow: hidden;
  height: 100%;
  width: 50px;
  background-color: #ebffcf;
}

.open-doors::before {
  display: block;
  box-sizing: border-box;
  content: "";
  width: 50%;
  height: 100%;
  position: absolute;
  top: 0;
  left: 0;
  background-color: gray;
  animation: opening-door linear 1.5s forwards;
}

.open-doors::after {
  display: block;
  content: "";
  width: 50%;
  height: 100%;
  position: absolute;
  top: 0;
  right: 0;
  background-color: gray;
  animation: opening-door linear 1.5s forwards;
}

@keyframes behind-doors {
  40% {
    height: 100%;
  }
  60% {
    height: 0;
  }
  100% {
    height: 0;
  }
}

@keyframes opening-door {
  0% {
    top: 100%;
    width: 50%;
  }

  40% {
    top: 0;
    width: 50%;
  }

  60% {
    top: 0;
    width: 50%;
  }

  100% {
    top: 0;
    width: 0;
  }
}


</style>
