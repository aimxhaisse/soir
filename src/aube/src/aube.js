import {EditorState} from "@codemirror/state"
import {EditorView, basicSetup} from "codemirror"

export default function aube() {
  let startState = EditorState.create({
    doc: "Hello World",
  })

  let view = new EditorView({
    state: startState,
    extensions: [basicSetup],
    parent: document.body
  })
}
