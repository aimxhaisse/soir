import {EditorState} from "@codemirror/state"
import {EditorView, basicSetup} from "codemirror"
import {python} from "@codemirror/lang-python"
import {oneDark} from "@codemirror/theme-one-dark"

export default function aube() {
  //let theme = EditorView.baseTheme({}, {dark: true});

  let state = EditorState.create({
    doc: "# Follow the white rabbit...",
    extensions: [
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
