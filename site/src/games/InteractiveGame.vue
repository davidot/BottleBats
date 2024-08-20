<template>
    Hello: {{ game }}
    <div v-show="!hasRunningGame">
        <h3>Setup new game of type {{ game }}</h3>

        <button>Quick start single player game</button>
        <table v-if="gameSetupConfig.numPlayers !== -1">
            <thead>
                <th>
                    <td>
                        Players
                    </td>
                </th>
            </thead>
            <tbody>
                <tr v-for="index in gameSetupConfig.numPlayers">
                    <td>
                        <label>Player {{ index }}</label>
                        <select v-model="pickedAlgos[index - 1]">
                            <option value="" selected>Player</option>
                            <template v-for="algo in gameSetupConfig.availableAlgos">
                                <option :value="algo">{{ algo }}</option>
                            </template>
                        </select>
                    </td>
                </tr>
                <tr>
                    <td>
                        <button>Start game!</button>
                    </td>
                </tr>
            </tbody>
        </table>
        <Console :messages="messages" />
    </div>
</template>

<script>
import { endpoint } from "@/http";
import Console from "./Console.vue";



export default {
    components: {
        Console,
    },
    props: {
        game: String,
    },
    mounted() {
        endpoint.get("/game-info/" + this.game)
            .then((val) => {
                console.log("Got val: " + val.data);
                this.gameSetupConfig = val.data;
                for (let i = 0; i < val.data.numPlayers; ++i) {
                    this.pickedAlgos.push("");
                }
            });
    },
    computed: {
        hasRunningGame() {
            return this.ws != null;
        }
    },
    data() {
        return {
            ws: null,
            gameSetupConfig: {
                numPlayers: -1,
                availableAlgos: [],
            },
            pickedAlgos: [],
            messages: [
                {from: "game", content: "Guessed 84 wrong!"},
                {from: "game", content: "Guessed 1234 wrong!"},
                {from: "game", content: "guess"},
                {from: "me", content: "123"},
            ]
        };
    },
};
</script>