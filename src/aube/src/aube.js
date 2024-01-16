import {EditorState} from "@codemirror/state"
import {indentWithTab} from "@codemirror/commands"
import {minimalSetup} from "codemirror"
import {EditorView, keymap} from "@codemirror/view"
import {python} from "@codemirror/lang-python"
import {oneDark} from "@codemirror/theme-one-dark"
import {emacs} from "@replit/codemirror-emacs"

export default function aube() {
 
  let state = EditorState.create({
    doc: "# Follow the white rabbit...",
    extensions: [
      emacs(),
      minimalSetup,
      python(),
      oneDark,
      keymap.of([indentWithTab]),
    ],
  })

  let view = new EditorView({
    state: state,
    parent: document.getElementById("code"),
  })
  
}
