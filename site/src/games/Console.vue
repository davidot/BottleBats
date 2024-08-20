<template>
    <div id="console">
        <span v-for="message in messages" :class="[message.from, 'message']">
            {{ message.content }}
        </span><br/>
        $> <span @select="print">{{ inputText }}</span><span id="text-input"><input type="text" v-model="inputText" ref="input" @select="print" @selectionchange="print"></span>
    </div>
</template>

<script>

export default {
    props: {
        messages: {
            type: Array,
            required: true,
        },
    },
    mounted() {
        document.addEventListener("select", (e) => {
            console.log('body select', e);
        })
        document.addEventListener("selectionchange", (e) => {
            console.log('body selectionchange', e);
            console.log(document.getSelection())
            console.log(document.getSelection().anchorNode)
        })
    },  
    data() {
        return {
            inputText: '',
        };
    },
    methods: {
        print(e) {
            console.log(e);
        }
    }

}
</script>

<style scoped>
#console {
    background-color: black;
    color: white;
    font-family: monospace;
    font-size: larger;
    padding-bottom: 4px;
}
/* :focus-within */
#text-input::before {
    content: '\00a0';
    width: 0.5em;
    height: 1em;
    color: orange;
    border-bottom: 2px solid white;
    animation: blink 1s step-end infinite;
}

#text-input > input {
    /* width: 0px; */
    overflow: hidden;
    border: 0;
    outline: 0;
    /* height: 0px; */
    /* opacity: 0; */
}

@keyframes blink {
  from,
  to {
    border-color: transparent;
  }
  50% {
    border-color: #fff;
  }
}

.message.me {
    color: green;
}

.message.game {
    color: red;
}
</style>