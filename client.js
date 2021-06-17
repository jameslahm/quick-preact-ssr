import App from "./app.js"
import {h, hydrate} from "./preact.js"
import htm from './htm.js'

const html = htm.bind(h);
hydrate(html`<${App} page="All" />`, document.body);