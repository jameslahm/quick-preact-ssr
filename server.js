import * as std from 'std'
import App from 'app.js'
import {h} from 'preact.js'
import htm from 'htm.js'
import preactRenderToString from 'preact-render-to-string.js'

const html = htm.bind(h);

std.out.puts(preactRenderToString(html`<${App} page="All" />`))