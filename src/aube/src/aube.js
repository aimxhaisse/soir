import {EditorState} from "@codemirror/state"
import {basicSetup, EditorView} from "codemirror"
import {python} from "@codemirror/lang-python"
import {oneDark} from "@codemirror/theme-one-dark"
import {emacs} from "@replit/codemirror-emacs"

export default function aube() {
 
  let state = EditorState.create({
    doc: "# Follow the white rabbit...",
    extensions: [
      emacs(),
      basicSetup,
      python(),
      oneDark,
    ],
  })

  let view = new EditorView({
    state: state,
    parent: document.getElementById("code"),
  })

}
