import htm from './htm.js'
import * as preact from './preact.js'

const {Component, h, hydrate} = preact
// Create your app
const html = htm.bind(preact.h);

class App extends Component {
    addTodo() {
      const { todos = [] } = this.state;
      this.setState({ todos: todos.concat(`Item ${todos.length}`) });
    }
    render({ page }, { todos = [] }) {
      return html`
        <div class="app">
          <${Header} name="ToDo's (${page})" />
          <ul>
            ${todos.map(todo => html`
              <li key="${todo}">${todo}</li>
            `)}
          </ul>
          <button onClick=${() => this.addTodo()}>Add Todo</button>
          <${Footer}>footer content here<//>
        </div>
      `;
    }
  }

const Header = ({ name }) => html`<h1>${name} List</h1>`

const Footer = props => html`<footer ...${props} />`

export default App