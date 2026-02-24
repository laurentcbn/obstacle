#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  HTML content (ton design, Web Audio retiré, pont JUCE ajouté)
// ─────────────────────────────────────────────────────────────────────────────
static juce::String buildHtml()
{
    return juce::String (R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>OBSTACLE // CBN</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Bebas+Neue&display=swap');

  :root {
    --bg: #050508;
    --surface: #0a0a10;
    --border: #1a1a2e;
    --accent: #00e5ff;
    --accent2: #ff0055;
    --dim: #1e2030;
    --text: #8899aa;
    --bright: #cce8f0;
  }

  * { margin: 0; padding: 0; box-sizing: border-box; }

  body {
    background: var(--bg);
    color: var(--bright);
    font-family: 'Share Tech Mono', monospace;
    min-height: 100vh;
    overflow-x: hidden;
    position: relative;
  }

  body::before {
    content: '';
    position: fixed;
    inset: 0;
    background: repeating-linear-gradient(
      0deg, transparent, transparent 2px,
      rgba(0,0,0,0.08) 2px, rgba(0,0,0,0.08) 4px
    );
    pointer-events: none;
    z-index: 100;
  }

  body::after {
    content: '';
    position: fixed;
    inset: 0;
    background: radial-gradient(ellipse at center, transparent 60%, rgba(0,0,0,0.7) 100%);
    pointer-events: none;
    z-index: 99;
  }

  .container { max-width: 900px; margin: 0 auto; padding: 40px 20px; }

  header { text-align: center; margin-bottom: 40px; position: relative; }

  h1 {
    font-family: 'Bebas Neue', sans-serif;
    font-size: clamp(60px, 10vw, 120px);
    letter-spacing: 0.2em;
    color: var(--accent);
    text-shadow: 0 0 30px rgba(0,229,255,0.4), 0 0 80px rgba(0,229,255,0.15);
    line-height: 1;
  }

  .subtitle { font-size: 11px; letter-spacing: 0.4em; color: var(--text); margin-top: 6px; text-transform: uppercase; }

  .bpm-row { display: flex; align-items: center; justify-content: center; gap: 20px; margin-bottom: 30px; }
  .bpm-label { font-size: 11px; letter-spacing: 0.3em; color: var(--text); }
  .bpm-value { font-size: 28px; color: var(--accent); width: 60px; text-align: center; }

  input[type=range] {
    -webkit-appearance: none; width: 160px; height: 2px;
    background: var(--border); outline: none; border-radius: 0;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none; width: 14px; height: 14px;
    background: var(--accent); border-radius: 0; cursor: pointer;
    box-shadow: 0 0 10px rgba(0,229,255,0.6);
  }

  .transport { display: flex; justify-content: center; gap: 16px; margin-bottom: 36px; }

  .btn {
    font-family: 'Share Tech Mono', monospace;
    font-size: 11px; letter-spacing: 0.3em; text-transform: uppercase;
    padding: 10px 28px; border: 1px solid var(--accent);
    background: transparent; color: var(--accent); cursor: pointer; transition: all 0.15s;
  }
  .btn:hover { background: rgba(0,229,255,0.08); box-shadow: 0 0 20px rgba(0,229,255,0.2); }
  .btn.active { background: var(--accent); color: var(--bg); box-shadow: 0 0 30px rgba(0,229,255,0.5); }
  .btn.stop-btn { border-color: var(--accent2); color: var(--accent2); }
  .btn.stop-btn:hover { background: rgba(255,0,85,0.08); box-shadow: 0 0 20px rgba(255,0,85,0.2); }

  .sequencer { border: 1px solid var(--border); padding: 20px; background: var(--surface); margin-bottom: 24px; }

  .track { display: grid; grid-template-columns: 90px 1fr; gap: 12px; align-items: center; margin-bottom: 12px; }
  .track:last-child { margin-bottom: 0; }

  .track-name { font-size: 10px; letter-spacing: 0.25em; color: var(--text); text-transform: uppercase; text-align: right; padding-right: 8px; border-right: 1px solid var(--border); }

  .steps { display: grid; grid-template-columns: repeat(16, 1fr); gap: 3px; }

  .step {
    aspect-ratio: 1; border: 1px solid var(--border); cursor: pointer;
    transition: all 0.1s; background: var(--dim); position: relative;
  }
  .step:hover { border-color: var(--text); }
  .step.on { background: var(--accent); border-color: var(--accent); box-shadow: 0 0 8px rgba(0,229,255,0.4); }
  .step.on.bass-step { background: #6644ff; border-color: #6644ff; box-shadow: 0 0 8px rgba(100,68,255,0.5); }
  .step.on.lead-step { background: #ff3388; border-color: #ff3388; box-shadow: 0 0 8px rgba(255,51,136,0.5); }
  .step:nth-child(4n) { margin-right: 4px; }
  .step.playing { outline: 2px solid rgba(255,255,255,0.6); outline-offset: 1px; }

  .note-row { display: grid; grid-template-columns: 90px 1fr; gap: 12px; align-items: center; margin-bottom: 4px; }
  .note-selects { display: grid; grid-template-columns: repeat(16, 1fr); gap: 3px; }

  select.note-sel {
    background: var(--dim); border: 1px solid var(--border); color: var(--text);
    font-family: 'Share Tech Mono', monospace; font-size: 8px;
    width: 100%; padding: 2px 0; text-align: center; cursor: pointer;
    -webkit-appearance: none; appearance: none; text-align-last: center;
  }
  select.note-sel:focus { outline: none; border-color: var(--accent); }

  .vu-row { display: flex; justify-content: center; gap: 4px; margin-bottom: 24px; height: 30px; align-items: flex-end; }
  .vu-bar { width: 8px; background: var(--dim); position: relative; overflow: hidden; }
  .vu-fill { position: absolute; bottom: 0; left: 0; right: 0; background: linear-gradient(to top, var(--accent), #ff0055); transition: height 0.05s; height: 0%; }

  .section-label { font-size: 9px; letter-spacing: 0.4em; color: var(--dim); text-transform: uppercase; margin-bottom: 8px; text-align: center; }

  .step-display { display: flex; justify-content: center; gap: 3px; margin-bottom: 20px; }
  .step-dot { width: 8px; height: 8px; border: 1px solid var(--border); background: var(--dim); transition: all 0.05s; }
  .step-dot.active { background: var(--accent); box-shadow: 0 0 6px rgba(0,229,255,0.6); }

  .knobs { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 24px; }
  .knob-group { display: flex; flex-direction: column; align-items: center; gap: 8px; }
  .knob-label { font-size: 9px; letter-spacing: 0.3em; color: var(--text); text-transform: uppercase; }
  .knob-val { font-size: 11px; color: var(--accent); }

  footer { text-align: center; margin-top: 40px; font-size: 9px; letter-spacing: 0.4em; color: var(--border); }
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>OBSTACLE</h1>
    <div class="subtitle">algorithmic sequencer // cbn edition // mkii</div>
  </header>

  <div class="bpm-row">
    <span class="bpm-label">BPM</span>
    <span class="bpm-value" id="bpmVal">128</span>
    <input type="range" id="bpmSlider" min="60" max="200" value="128">
    <span class="bpm-label">KEY&nbsp;</span>
    <select id="keySelect" style="background:var(--dim);border:1px solid var(--border);color:var(--accent);font-family:'Share Tech Mono',monospace;font-size:12px;padding:4px 8px;">
      <option value="-12">A-1</option><option value="-11">Bb-1</option><option value="-10">B-1</option>
      <option value="-9">C-1</option><option value="-8">C#-1</option><option value="-7">D-1</option>
      <option value="-6">Eb-1</option><option value="-5">E-1</option><option value="-4">F-1</option>
      <option value="-3">F#-1</option><option value="-2">G-1</option><option value="-1">Ab-1</option>
      <option value="0" selected>A</option>
      <option value="1">Bb</option><option value="2">B</option><option value="3">C</option>
      <option value="4">C#</option><option value="5">D</option><option value="6">Eb</option>
      <option value="7">E</option><option value="8">F</option><option value="9">F#</option>
      <option value="10">G</option><option value="11">Ab</option><option value="12">A+1</option>
    </select>
  </div>

  <div class="transport">
    <button class="btn" id="playBtn" onclick="togglePlay()">&#9654; PLAY</button>
    <button class="btn stop-btn" onclick="stopSeq()">&#9632; STOP</button>
    <button class="btn" onclick="randomize()" style="border-color:#6644ff;color:#6644ff;">&#10227; REGEN</button>
  </div>

  <div class="step-display" id="stepDisplay"></div>
  <div class="sequencer" id="sequencer"></div>

  <div class="knobs">
    <div class="knob-group">
      <span class="knob-label">Reverb</span>
      <input type="range" min="0" max="100" value="40" id="reverbKnob" oninput="updateFX()">
      <span class="knob-val" id="reverbVal">40%</span>
    </div>
    <div class="knob-group">
      <span class="knob-label">Delay</span>
      <input type="range" min="0" max="100" value="39" id="delayKnob" oninput="updateFX()">
      <span class="knob-val" id="delayVal">39%</span>
    </div>
    <div class="knob-group">
      <span class="knob-label">Cutoff</span>
      <input type="range" min="500" max="20000" value="8000" id="cutoffKnob" oninput="updateFX()">
      <span class="knob-val" id="cutoffVal">8000Hz</span>
    </div>
    <div class="knob-group">
      <span class="knob-label">Drive</span>
      <input type="range" min="1" max="20" value="3" id="driveKnob" oninput="updateFX()">
      <span class="knob-val" id="driveVal">3x</span>
    </div>
  </div>

  <div class="vu-row" id="vuRow"></div>

  <footer>OBSTACLE ENGINE v2.0 // JUCE AUDIO // CBN</footer>
</div>

<script>
// ── JUCE Bridge ──────────────────────────────────────────────────────────────
let _juceCallId = 0;

function juceSend (name) {
  var args = Array.prototype.slice.call(arguments, 1);
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.emitEvent('__juce__invoke', { name: name, params: args, resultId: _juceCallId++ });
  }
}

function juceAsync (name) {
  var args = Array.prototype.slice.call(arguments, 1);
  return new Promise(function(resolve) {
    if (!window.__JUCE__ || !window.__JUCE__.backend) { resolve(null); return; }
    var callId = _juceCallId++;
    function handler(data) {
      if (data && data.promiseId === callId) {
        window.__JUCE__.backend.removeEventListener('__juce__complete', handler);
        resolve(data.result);
      }
    }
    window.__JUCE__.backend.addEventListener('__juce__complete', handler);
    window.__JUCE__.backend.emitEvent('__juce__invoke', { name: name, params: args, resultId: callId });
  });
}

function waitForJuce (cb, ms) {
  ms = ms || 4000;
  if (window.__JUCE__ && window.__JUCE__.backend) { cb(); return; }
  var t0 = Date.now();
  var iv = setInterval(function() {
    if ((window.__JUCE__ && window.__JUCE__.backend) || Date.now() - t0 > ms) {
      clearInterval(iv);
      if (window.__JUCE__ && window.__JUCE__.backend) cb();
    }
  }, 50);
}

// ── Sequencer State ──────────────────────────────────────────────────────────
var STEPS = 16;
var tracks = [
  { id: 'kick',  label: 'KICK',   type: 'drum',    pattern: new Array(STEPS).fill(false) },
  { id: 'snare', label: 'SNARE',  type: 'drum',    pattern: new Array(STEPS).fill(false) },
  { id: 'hihat', label: 'HI-HAT', type: 'drum',    pattern: new Array(STEPS).fill(false) },
  { id: 'bass',  label: 'BASS',   type: 'melodic', pattern: new Array(STEPS).fill(false), notes: new Array(STEPS).fill(0) },
  { id: 'lead',  label: 'LEAD',   type: 'melodic', pattern: new Array(STEPS).fill(false), notes: new Array(STEPS).fill(4) },
  { id: 'pad',   label: 'PAD',    type: 'melodic', pattern: new Array(STEPS).fill(false), notes: new Array(STEPS).fill(2) },
];

function loadDefaultPattern() {
  tracks[0].pattern = [1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0].map(Boolean);
  tracks[1].pattern = [0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0].map(Boolean);
  tracks[2].pattern = [1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1].map(Boolean);
  tracks[3].pattern = [1,0,0,1,0,0,1,0,1,0,0,1,0,1,0,0].map(Boolean);
  tracks[3].notes   = [0,0,0,5,0,0,3,0,0,0,0,0,0,5,0,0];
  tracks[4].pattern = [0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1].map(Boolean);
  tracks[4].notes   = [4,4,4,4,4,4,6,4,4,4,4,4,4,4,4,2];
  tracks[5].pattern = [1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0].map(Boolean);
  tracks[5].notes   = [2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2];
}
loadDefaultPattern();

var bassNotes = ['C2','D2','Eb2','F2','G2','Ab2','Bb2'];
var leadNotes = ['C4','D4','Eb4','F4','G4','Ab4','Bb4'];
var uiPlaying = false;

// ── Build UI ─────────────────────────────────────────────────────────────────
function buildUI() {
  var seq = document.getElementById('sequencer');
  seq.innerHTML = '';

  tracks.forEach(function(track, ti) {
    var row = document.createElement('div');
    row.className = 'track';
    row.innerHTML = '<div class="track-name">' + track.label + '</div><div class="steps" id="steps-' + track.id + '"></div>';
    seq.appendChild(row);

    var stepsDiv = row.querySelector('.steps');
    for (var s = 0; s < STEPS; s++) {
      (function(s_) {
        var btn = document.createElement('div');
        btn.className = 'step';
        if (track.pattern[s_]) {
          btn.classList.add('on');
          if (track.id === 'bass') btn.classList.add('bass-step');
          if (track.id === 'lead' || track.id === 'pad') btn.classList.add('lead-step');
        }
        btn.onclick = function() { toggleStep(ti, s_); };
        stepsDiv.appendChild(btn);
      })(s);
    }

    if (track.type === 'melodic') {
      var noteRow = document.createElement('div');
      noteRow.className = 'note-row';
      noteRow.innerHTML = '<div class="track-name" style="font-size:8px;color:#1a1a2e">NOTES</div><div class="note-selects" id="notes-' + track.id + '"></div>';
      seq.appendChild(noteRow);
      var noteDiv = noteRow.querySelector('.note-selects');
      var noteArr = (track.id === 'bass') ? bassNotes : leadNotes;
      for (var s2 = 0; s2 < STEPS; s2++) {
        (function(s2_, ti_) {
          var sel = document.createElement('select');
          sel.className = 'note-sel';
          noteArr.forEach(function(n, ni) {
            var opt = document.createElement('option');
            opt.value = ni;
            opt.textContent = n.replace('b','♭');
            sel.appendChild(opt);
          });
          sel.value = tracks[ti_].notes[s2_];
          sel.onchange = function() {
            var v = parseInt(sel.value);
            tracks[ti_].notes[s2_] = v;
            juceSend('juceNote', ti_, s2_, v);
          };
          noteDiv.appendChild(sel);
        })(s2, ti);
      }
    }
  });

  // Step dots
  var sd = document.getElementById('stepDisplay');
  sd.innerHTML = '';
  for (var i = 0; i < STEPS; i++) {
    var d = document.createElement('div');
    d.className = 'step-dot';
    d.id = 'dot-' + i;
    sd.appendChild(d);
  }

  // VU bars
  var vu = document.getElementById('vuRow');
  vu.innerHTML = '';
  for (var j = 0; j < 24; j++) {
    var bar = document.createElement('div');
    bar.className = 'vu-bar';
    bar.innerHTML = '<div class="vu-fill" id="vu-' + j + '"></div>';
    vu.appendChild(bar);
  }
}

// ── Interactions ─────────────────────────────────────────────────────────────
function toggleStep(ti, s) {
  tracks[ti].pattern[s] = !tracks[ti].pattern[s];
  var stepsDiv = document.getElementById('steps-' + tracks[ti].id);
  if (stepsDiv) {
    var btn = stepsDiv.querySelectorAll('.step')[s];
    if (btn) {
      if (tracks[ti].pattern[s]) {
        btn.classList.add('on');
        if (tracks[ti].id === 'bass') btn.classList.add('bass-step');
        if (tracks[ti].id === 'lead' || tracks[ti].id === 'pad') btn.classList.add('lead-step');
      } else {
        btn.classList.remove('on','bass-step','lead-step');
      }
    }
  }
  juceSend('juceToggle', ti, s);
}

function togglePlay() {
  juceSend('jucePlay');
  uiPlaying = !uiPlaying;
  var btn = document.getElementById('playBtn');
  if (uiPlaying) {
    btn.classList.add('active');
    btn.textContent = '\u275A\u275A PAUSE';
  } else {
    btn.classList.remove('active');
    btn.textContent = '\u25BA PLAY';
    updateCurrentStep(-1);
  }
}

function stopSeq() {
  juceSend('juceStop');
  uiPlaying = false;
  document.getElementById('playBtn').classList.remove('active');
  document.getElementById('playBtn').textContent = '\u25BA PLAY';
  updateCurrentStep(-1);
}

function randomize() {
  juceAsync('juceRandomize').then(function(result) {
    if (!result) return;
    for (var ti = 0; ti < tracks.length; ti++) {
      if (result[ti]) {
        if (result[ti].pattern) tracks[ti].pattern = Array.prototype.slice.call(result[ti].pattern).map(Boolean);
        if (result[ti].notes)   tracks[ti].notes   = Array.prototype.slice.call(result[ti].notes).map(Number);
      }
    }
    buildUI();
  });
}

function updateFX() {
  var rv = parseInt(document.getElementById('reverbKnob').value);
  var dl = parseInt(document.getElementById('delayKnob').value);
  var cf = parseInt(document.getElementById('cutoffKnob').value);
  var dr = parseInt(document.getElementById('driveKnob').value);
  document.getElementById('reverbVal').textContent = rv + '%';
  document.getElementById('delayVal').textContent  = dl + '%';
  document.getElementById('cutoffVal').textContent = cf + 'Hz';
  document.getElementById('driveVal').textContent  = dr + 'x';
  juceSend('juceParam', 'reverb', rv);
  juceSend('juceParam', 'delay',  dl);
  juceSend('juceParam', 'cutoff', cf);
  juceSend('juceParam', 'drive',  dr);
}

function updateCurrentStep(step) {
  document.querySelectorAll('.step.playing').forEach(function(el) { el.classList.remove('playing'); });
  document.querySelectorAll('.step-dot').forEach(function(el) { el.classList.remove('active'); });
  if (step < 0) {
    document.querySelectorAll('.vu-fill').forEach(function(el) { el.style.height = '0%'; });
    return;
  }
  tracks.forEach(function(track) {
    var stepsDiv = document.getElementById('steps-' + track.id);
    if (stepsDiv) {
      var stepEls = stepsDiv.querySelectorAll('.step');
      if (stepEls[step]) stepEls[step].classList.add('playing');
    }
  });
  var dot = document.getElementById('dot-' + step);
  if (dot) dot.classList.add('active');
  document.querySelectorAll('.vu-fill').forEach(function(bar) {
    var h = Math.random() * 65 + 5;
    bar.style.height = h + '%';
    setTimeout(function() { bar.style.height = '0%'; }, 100);
  });
}

// ── Slider / Select handlers ─────────────────────────────────────────────────
document.getElementById('bpmSlider').oninput = function() {
  document.getElementById('bpmVal').textContent = this.value;
  juceSend('juceParam', 'bpm', parseFloat(this.value));
};
document.getElementById('keySelect').onchange = function() {
  juceSend('juceParam', 'key', parseInt(this.value));
};

// ── Init UI then connect to JUCE ─────────────────────────────────────────────
buildUI();

waitForJuce(function() {
  // C++ → JS: step counter
  window.__JUCE__.backend.addEventListener('stepUpdate', function(step) {
    updateCurrentStep(parseInt(step));
  });

  // C++ → JS: play state sync
  window.__JUCE__.backend.addEventListener('playStateUpdate', function(playing) {
    uiPlaying = !!playing;
    var btn = document.getElementById('playBtn');
    if (uiPlaying) {
      btn.classList.add('active');
      btn.textContent = '\u275A\u275A PAUSE';
    } else {
      btn.classList.remove('active');
      btn.textContent = '\u25BA PLAY';
    }
  });

  // Request initial state from C++
  juceAsync('juceGetState').then(function(state) {
    if (!state) return;
    if (state.bpm !== undefined) {
      document.getElementById('bpmSlider').value = state.bpm;
      document.getElementById('bpmVal').textContent = Math.round(state.bpm);
    }
    if (state.key !== undefined) {
      document.getElementById('keySelect').value = state.key;
    }
    if (state.reverb !== undefined) {
      var rv = Math.round(state.reverb * 100);
      document.getElementById('reverbKnob').value = rv;
      document.getElementById('reverbVal').textContent = rv + '%';
    }
    if (state.delay !== undefined) {
      var dl = Math.round(state.delay * 100);
      document.getElementById('delayKnob').value = dl;
      document.getElementById('delayVal').textContent = dl + '%';
    }
    if (state.cutoff !== undefined) {
      document.getElementById('cutoffKnob').value = Math.round(state.cutoff);
      document.getElementById('cutoffVal').textContent = Math.round(state.cutoff) + 'Hz';
    }
    if (state.drive !== undefined) {
      var dr = Math.round(state.drive);
      document.getElementById('driveKnob').value = dr;
      document.getElementById('driveVal').textContent = dr + 'x';
    }
    if (state.patterns) {
      for (var ti = 0; ti < tracks.length; ti++) {
        if (state.patterns[ti]) {
          if (state.patterns[ti].pattern)
            tracks[ti].pattern = Array.prototype.slice.call(state.patterns[ti].pattern).map(Boolean);
          if (state.patterns[ti].notes)
            tracks[ti].notes = Array.prototype.slice.call(state.patterns[ti].notes).map(Number);
        }
      }
      buildUI();
    }
  });
});
</script>
</body>
</html>
)HTML");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor — WebBrowserComponent with native functions
// ─────────────────────────────────────────────────────────────────────────────
ObstacleEditor::ObstacleEditor (ObstacleProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      webView (juce::WebBrowserComponent::Options{}
                   .withNativeIntegrationEnabled()
                   .withResourceProvider (
                       [this] (const auto& url) { return getResource (url); },
                       juce::URL { juce::WebBrowserComponent::getResourceProviderRoot() }.getOrigin())
                   // ── Toggle a step ─────────────────────────────────────────
                   .withNativeFunction ("juceToggle",
                       [this] (const juce::var& args, auto complete) {
                           int ti = (int)args[0], s = (int)args[1];
                           if (ti >= 0 && ti < NUM_TRACKS && s >= 0 && s < 16)
                               proc.steps[ti][s] = !proc.steps[ti][s];
                           complete (juce::var{});
                       })
                   // ── Set melodic note ──────────────────────────────────────
                   .withNativeFunction ("juceNote",
                       [this] (const juce::var& args, auto complete) {
                           int ti = (int)args[0], s = (int)args[1], v = (int)args[2];
                           if (ti >= 0 && ti < NUM_TRACKS && s >= 0 && s < 16)
                               proc.stepNotes[ti][s] = juce::jlimit (0, 6, v);
                           complete (juce::var{});
                       })
                   // ── Play / Stop ───────────────────────────────────────────
                   .withNativeFunction ("jucePlay",
                       [this] (const juce::var&, auto complete) {
                           proc.playing.store (!proc.playing.load());
                           complete (juce::var{});
                       })
                   .withNativeFunction ("juceStop",
                       [this] (const juce::var&, auto complete) {
                           proc.playing.store (false);
                           complete (juce::var{});
                       })
                   // ── Randomize and return new pattern ─────────────────────
                   .withNativeFunction ("juceRandomize",
                       [this] (const juce::var&, auto complete) {
                           proc.randomizePattern();
                           complete (buildPatternVar());
                       })
                   // ── Parameter change ──────────────────────────────────────
                   .withNativeFunction ("juceParam",
                       [this] (const juce::var& args, auto complete) {
                           setParam (args[0].toString(), (float)args[1]);
                           complete (juce::var{});
                       })
                   // ── Initial state request ─────────────────────────────────
                   .withNativeFunction ("juceGetState",
                       [this] (const juce::var&, auto complete) {
                           complete (buildStateVar());
                       }))
{
    addAndMakeVisible (webView);
    setResizable (true, false);
    setResizeLimits (800, 480, 1680, 1040);
    setSize (1100, 640);
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz (30);
}

ObstacleEditor::~ObstacleEditor()
{
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleEditor::timerCallback()
{
    int  step    = proc.currentStep.load();
    bool playing = proc.playing.load();

    if (step != lastStep) {
        lastStep = step;
        webView.emitEventIfBrowserIsVisible ("stepUpdate",
            juce::var (playing ? step : -1));
    }
    if (playing != uiPlaying) {
        uiPlaying = playing;
        webView.emitEventIfBrowserIsVisible ("playStateUpdate", juce::var (playing));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resource provider — serves our HTML
// ─────────────────────────────────────────────────────────────────────────────
std::optional<juce::WebBrowserComponent::Resource>
ObstacleEditor::getResource (const juce::String& url)
{
    juce::String path = (url == "/")
        ? "index.html"
        : url.fromFirstOccurrenceOf ("/", false, false);

    if (path.isEmpty() || path == "index.html")
    {
        auto html = buildHtml();
        const auto numBytes = html.getNumBytesAsUTF8();
        std::vector<std::byte> data (numBytes);
        std::memcpy (data.data(), html.toRawUTF8(), numBytes);
        return juce::WebBrowserComponent::Resource { std::move (data), "text/html" };
    }
    return std::nullopt;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Build pattern juce::var (for JS)
// ─────────────────────────────────────────────────────────────────────────────
juce::var ObstacleEditor::buildPatternVar() const
{
    juce::Array<juce::var> result;
    for (int t = 0; t < NUM_TRACKS; ++t)
    {
        auto* obj = new juce::DynamicObject();
        juce::Array<juce::var> pat, notes;
        for (int s = 0; s < 16; ++s) {
            pat.add   (juce::var (proc.steps[t][s]));
            notes.add (juce::var (proc.stepNotes[t][s]));
        }
        obj->setProperty ("pattern", juce::var (pat));
        obj->setProperty ("notes",   juce::var (notes));
        result.add (juce::var (obj));
    }
    return juce::var (result);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Build full state juce::var (for initial sync)
// ─────────────────────────────────────────────────────────────────────────────
juce::var ObstacleEditor::buildStateVar() const
{
    auto* obj = new juce::DynamicObject();
    if (proc.bpmParam)       obj->setProperty ("bpm",    (double)proc.bpmParam->get());
    if (proc.reverbParam)    obj->setProperty ("reverb", (double)proc.reverbParam->get());
    if (proc.delayMixParam)  obj->setProperty ("delay",  (double)proc.delayMixParam->get());
    if (proc.filterCutParam) obj->setProperty ("cutoff", (double)proc.filterCutParam->get());
    if (proc.driveParam)     obj->setProperty ("drive",  (double)juce::jlimit (1.f, 20.f, proc.driveParam->get() * 2.f));
    if (proc.keyParam)       obj->setProperty ("key",    (int)proc.keyParam->get());
    obj->setProperty ("patterns", buildPatternVar());
    return juce::var (obj);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Set parameter from JS
// ─────────────────────────────────────────────────────────────────────────────
void ObstacleEditor::setParam (const juce::String& name, float value)
{
    auto setNorm = [] (juce::AudioParameterFloat* p, float v) {
        if (p) p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (v));
    };

    if      (name == "bpm")    setNorm (proc.bpmParam,       juce::jlimit (60.f, 200.f, value));
    else if (name == "reverb") setNorm (proc.reverbParam,     juce::jlimit (0.f,  1.f,   value / 100.f));
    else if (name == "delay")  setNorm (proc.delayMixParam,   juce::jlimit (0.f,  0.9f,  value / 100.f * 0.9f));
    else if (name == "cutoff") setNorm (proc.filterCutParam,  juce::jlimit (500.f, 20000.f, value));
    else if (name == "drive")  setNorm (proc.driveParam,      juce::jlimit (0.5f, 10.f, value * 0.5f));
    else if (name == "key" && proc.keyParam)
        proc.keyParam->setValueNotifyingHost (
            proc.keyParam->getNormalisableRange().convertTo0to1 (
                juce::jlimit (-12.f, 12.f, value)));
}
