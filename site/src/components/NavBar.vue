<template>
  <div class="top-bar">
    <div class="open-doors" style="height: 100%; float: left; z-index: 1; border: 0">
      <div style="width: 100%; height: 100%; background-color: #ebffcf; animation: behind-doors step-end 1.5s forwards">
      </div>
      <LogoSVG class="logo-nav" style="height: 100%; z-index: 1"/>
    </div>
    <span class="nav-button logo-nav" style="border-left-width: 0;">
      Bottle Bats
    </span>

    <NavBarButton to="leaderboard">
      LeaderBoard
    </NavBarButton>
    <NavBarButton to="play">
      Play / Watch
    </NavBarButton>
    <NavBarButton to="bots">
      My bots
    </NavBarButton>
    <NavBarButton to="rules">
      Rules
    </NavBarButton>
    <NavBarButton to="admin" v-if="isAdmin">
      Configure
    </NavBarButton>

    <NavBarButton to="login" :clazz="'login-side'" v-if="!loggedIn">
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

    <div class="login-side nav-button nav-with-menu" @click.self="toggleGame">
      <a>
        {{ gameList[$route.params.game]?.name ?? '???' }}
      </a>
      <div class="nav-dropdown" :style="{'visibility': pickGame ? 'visible' : '', 'opacity': pickGame ? '1' : ''}">
        <template v-for="(gameInfo, gameName) in gameList">
          <div @click="selectGame(gameName)" v-if="gameName != $route.params.game">
            {{ gameInfo.name }}
          </div>
        </template>
      </div>
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
  inject: ["userDetails", "gameList"],
  computed: {
    loggedIn() {
      return this.userDetails.values.value.displayName != null;
    },
    isAdmin() {
      return this.userDetails.values.value.isAdmin === true;
    },
    username() {
      if (this.loggedIn) return this.userDetails.values.value.displayName;
      return "Not logged in";
    },
  },
  data() {
    return {
      selectedGame: this.$route.game,
      pickGame: false,
    };
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
    },
    selectGame(newGame) {
      this.$router.push({
          name: this.$route.name,
          params: {
            game: newGame,
          }
        });
        this.pickGame = false;
    },
    toggleGame() {
      // If currently no game, just prevent the menu from opening
      if (!this.pickGame && this.$route.params.game == null)
        return;

      this.pickGame = !this.pickGame;
    },
  },
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

.nav-dropdown {
  position: absolute;
  top: 100%;
  visibility: hidden;
  opacity: 0;

  transition: all 0.1s;
  width: 100%;
}

.nav-dropdown > div {
  min-height: 50px;

  background: #5156bf;
  border: 1px solid white;
  color: white;
  align-content: center;
}

.nav-with-menu {
  position: relative;
  box-sizing: border-box;
  min-width: 100px;
}


</style>
