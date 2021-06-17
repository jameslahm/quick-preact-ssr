## Quick-Preact-SSR
Inspired By https://github.com/saghul/njk. Fast Preact SSR.

### Features
- low overhead and very small (use quickjs)
- pre-compile dependencies(include preact and htm, and also other components if we want) to bytecode, fast in ssr
- share code with client, no redundancy

### Get Started
```bash
make
# qpreact <port> <ssr-entry>
./build/qpreact-linux-x86_64 5000 server.js
```
Then, visit http://127.0.0.1:5000/, you can find the preact todo mvc in ssr.

## Others
I change `quickjs` a little bit. So it's in here rather than submodule.
