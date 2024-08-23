<template>
    Hello: {{ game }}
    <div v-show="!hasRunningGame">
        <h3>Setup new game of type {{ game }}</h3>

        <button v-if="!hasRunningGame" @click="startSingleplayer">Quick start single player game</button>
        <!-- <table v-if="gameSetupConfig.numPlayers !== -1 && !hasRunningGame">
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
        </table> -->
    </div>
    <hr>
    <div class="foldable">
        <div v-if="!gameFolded">
            <span v-if="!hasRunningGame || messages.length === 0">
                Waiting for game to start...
            </span>
            <component v-else :is="gameComponent" :messages="messages" @suggestion="setSuggestion" @sendMessage="pushMessage"/>

        </div>
    </div>
    <hr>
    <Console :messages="messages" :waiting-on-us="waitingOnUs" :suggestion="suggestion" @sendMessage="pushMessage"/>
</template>

<script>
import { endpoint } from "@/http";
import Console from "./Console.vue";

// Game imports
import GuessGame from "@/games/GuessGame.vue"



export default {
    components: {
        Console,
        GuessGame
    },
    props: {
        game: String,
    },
    mounted() {
        // endpoint.get("/game-info/" + this.game)
        //     .then((val) => {
        //         console.log("Got val: " + val.data);
        //         this.gameSetupConfig = val.data;
        //         for (let i = 0; i < val.data.numPlayers; ++i) {
        //             this.pickedAlgos.push("");
        //         }
        //     });
        setTimeout(() => {

            this.gameSetupConfig = {
                numPlayers: 5,
                availableAlgos: ['internal'],
                gameBaseName: 'Guess',
            }

            const lines = [
                "result 750 lower",
                "result 335 incorrect",
                "result 334 incorrect",
                "result 335 incorrect",
                "result 346 incorrect",
                "result 250 higher",
                "result 336 incorrect",
                "result 335 incorrect",
                "result 336 incorrect",
                "result 347 incorrect",
                "result 700 lower",
                "result 337 incorrect",
                "result 336 incorrect",
                "result 337 incorrect",
                "result 348 incorrect",
            ];

            for (let line of lines) {
                this.messages.push({from: "game", content: line});
            }
        }, 500);
    },
    computed: {
        hasRunningGame() {
            return this.ws != null || this.messages.length > 0;
        },
        gameComponent() {
            if (!this.gameSetupConfig.gameBaseName)
                return null;

            return this.gameSetupConfig.gameBaseName + 'Game';
        }
    },
    data() {
        return {
            ws: null,
            waitingOnUs: false,
            suggestion: '',
            gameFolded: false,
            gameSetupConfig: {
                numPlayers: -1,
                availableAlgos: [],
                gameBaseName: null,
            },
            pickedAlgos: [],
            messages: []
        };
    },
    methods: {
        pushMessage(mess) {
            if (this.ws != null) {
                mess = mess.trim();
                if (mess !== '')
                    mess = mess + '\n';
                this.ws.send(mess);
                this.messages.push({from: "me", content: mess.trim()});
                this.waitingOnUs = false;
            }

            // this.messages.push({from: "me", content: mess});
            // setTimeout(() => {
            //     this.messages.push({from: "game", content: "guess 123"});
            //     setTimeout(() => {
            //         this.messages.push({from: "game", content: "guess 456"});
            //     }, 50);
            //     setTimeout(() => {
            //         this.messages.push({from: "game", content: "guess 789"});
            //     }, 50)
            // }, 500);
        },
        connectToGame(matchCode) {
            if (this.ws) {
                console.log("Already plaing game?");
                return;
            }

            const ws = new WebSocket("ws://" + window.location.host + "/ws-api/game-join?match=" + encodeURIComponent(matchCode));
            // const ws = new WebSocket("ws://localhost:18081/ws-api/game-join?match=" + encodeURIComponent(matchCode));
            ws.onclose = (ev) => {
                console.log("Closed ws due to " + ev.reason + " (" + ev.statusCode + ")");
                if (this.ws === ws)
                    this.ws = null;
            };
            ws.onopen = (ev) => {
                console.log("Got connection in WS");
                this.messages = [
                    {from: "system", content: "Connected to " + matchCode},
                ];
                this.ws = ws;
            };
            ws.onerror = (ev) => {
                console.log("Got error on WS:", ev)
            };
            ws.onmessage = (ev) => {
                console.log("Got message: " + ev.data);
                this.messages.push({from: "game", content: ev.data});
                this.waitingOnUs = true;
            };
        },
        startSingleplayer() {
            this.connectToGame(this.game + ";S");
        },
        setSuggestion(hint) {
            this.suggestion = hint;
        }
    }
};
</script>