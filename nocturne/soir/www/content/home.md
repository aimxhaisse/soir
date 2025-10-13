<h1>Soir
  <div class="haiku-container">
    <i>An old silent pond<br />
      A frog jumps into the pond<br />
      Silence again.</i> -- <b>Bashô</b>
  </div>
</h1>

Live coding platform for sonic exploration.

## Typography Examples

This is a paragraph with regular text. It demonstrates the base typography styles. Links look like [this example link](#) and should have proper hover states.

### Heading Level 3

#### Heading Level 4

##### Heading Level 5

###### Heading Level 6

## Lists

### Unordered List
- First item in unordered list
- Second item with some longer text to test wrapping
- Third item
  - Nested item one
  - Nested item two
    - Double nested item

### Ordered List
1. First numbered item
2. Second numbered item with longer content
3. Third numbered item
   1. Nested numbered item
   2. Another nested item

## Code Examples

Here's some `inline code` within a paragraph.

```python
def hello_world():
    """A simple function to demonstrate code blocks."""
    message = "Hello, Soir!"
    print(message)
    return message

# This is a comment
for i in range(3):
    hello_world()
```

```javascript
// JavaScript example
const soirEngine = {
    frequency: 440,
    play() {
        console.log(`Playing tone at ${this.frequency}Hz`);
    }
};
```

## Tables

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| frequency | number | 440 | The fundamental frequency in Hz |
| amplitude | number | 0.5 | Amplitude from 0.0 to 1.0 |
| waveform | string | "sine" | Waveform type: sine, square, sawtooth |
| duration | number | 1.0 | Duration in seconds |

## Blockquotes

> This is a blockquote to demonstrate quote styling.
> It can span multiple lines and should have proper
> visual separation from the main content.

> Single line blockquote for comparison.

## Mixed Content

Here's a paragraph followed by code, then a table, demonstrating how different elements flow together:

```bash
# Install Soir
uv sync --all-extras
just build
uv run soir --version
```

And here's a small data table:

| Command | Purpose |
|---------|---------|
| `just build` | Build the project |
| `just test` | Run tests |
| `uv run soir` | Start Soir CLI |
