import axios from "axios";

const endpoint = axios.create({
    baseURL: '/api',
});

export { endpoint };
