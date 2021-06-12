// The contents of this class allow you to configure Rex.
class Rex {
  constructor() {
    this._dependencies = 0;

    // Optional elements on the page.
    this._status_element = document.getElementById('status');
    this._progress_element = document.getElementById('progress');
    this._spinner_element = document.getElementById('spinner');

    // Mandatory elements on the page.
    this._canvas_element = document.getElementById('canvas');

    if (this._canvas_element) {
      this._canvas_element.addEventListener("webglcontextlost", Rex._webgl_context_loss, false);
    } else {
      alert('Rex needs a canvas element to render to!');
    }
  }

  _update_status(text) {
    // Set the initial status when none present.
    if (!this._status) {
      this._status = { time: Date.now(), text: text };
    }

    // Contents have not changed since last update.
    if (this._status.text === text) {
       return;
    }

    const match = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    const time = Date.now();

    // Progress update is too soon, return.
    if (match && time - this._status.time < 30) {
      return;
    }

    this._status = { time: Date.now(), text: text };

    if (match) {
      text = match[1];
      const value = parseInt(match[2], 10) * 100;
      const max = parseInt(match[4], 10) * 100;
      if (this._progress_element) {
        this._progress_element.value = value;
        this._progress_element.max = max;
        this._progress_element.hidden = false;
      }

      if (this._spinner_element) {
        this._spinner_element.hidden = false;
      }
    } else {
      if (this._progress_element) {
        this._progress_element.value = null;
        this._progress_element.max = null;
        this._progress_element.hidden = true;
      }

      if (!text && this._spinner_element) {
        this._spinner_element.hidden = true;
      }
    }

    if (this._status_element) {
      this._status_element.innerHTML = text;
    }
  }

  _monitor_dependencies(left) {
    this._dependencies = Math.max(this._dependencies, left);
    if (left) {
      this._update_status(`Preparing... (${this._dependencies - left}/${this._dependencies})`);
    } else {
      this._update_status('All downloads complete.');
    }
  }

  static _webgl_context_loss(event) {
    // TODO(dweiler): Send the event to Rex and have Rex correctly restart when
    // encountering a loss of the WebGL context.
    alert('WebGL context lost. You will need to reload the page.');
    event.preventDefault();
  }

  static _locate_file(path, prefix) {
    // Specify custom file location code here. This function will let you replace
    // specific files and paths to go to other URLs, such as a CDN.
    //
    // Here we just return path as everything is loaded relative to this page.
    return path;
  }

  module() {
    this._update_status('Downloading...');
    return {
      locateFile:             Rex._locate_file,
      canvas:                 this._canvas_element,
      setStatus:              this._update_status.bind(this),
      monitorRunDependencies: this._monitor_dependencies.bind(this)
    };
  }
};

var Module = new Rex().module();