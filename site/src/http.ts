import axios from "axios";

const endpoint = axios.create({
    baseURL: '/api',
});

console.log('Http: ');
console.log(import.meta.env.BACKEND_BASE_URL);

export { endpoint };
