<template>
    <a :href="downloadURL" download="pieces.json">
        <slot></slot>
    </a>
</template>

<script setup>
import rawPieces from '@/components/blokus/pieces.json';

const downloadURL = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(rawPieces, null, 4));
</script>