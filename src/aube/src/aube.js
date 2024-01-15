import {EditorState} from "@codemirror/state"
import {minimalSetup, EditorView} from "codemirror"
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
    ],
  })

  let view = new EditorView({
    state: state,
    parent: document.getElementById("code"),
  })

  const fixedHeightEditor = EditorView.theme({
    "&": {height: "300px"},
    ".cm-scroller": {overflow: "auto"}
  })
  
}
