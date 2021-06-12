# Web

Skeleton files to setup a minimal Rex web page with download progress.

| File          | Description                                               |
|---------------|-----------------------------------------------------------|
| rex.css       | Default style rules for Rex canvas and download progress. |
| rex.module.js | Module configuration, all configuration can be done here. |
| shell.html    | A shell HTML page with the minimum elements necessary.    |

In addition to the above files, the following additional files will be
generated as well.

| File          | Description                                               |
|---------------|-----------------------------------------------------------|
| rex.js        | Startup JS that loads and sets up WebAssembly.            |
| rex.worker.js | WebWorker JS that loads and sets up threading.            |
| rex.wasm      | Compiled Rex binary as WebAssembly.                       |
| rex.data      | Asset archive. All runtime assets are stored here.        |

> Only the three skeleton files can be modified for desired output.