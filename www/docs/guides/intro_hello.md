# Hello World

The idea behind live coding is to write and evaluate code to change a
system incrementally. Let's do our very first session, go in the Neon
`live` directory and with your favourite editor, open the file
`hello.py`:
 
``` python
# NEON session

log('Hello World')
```

Neon is watching this directory and whenever something changes in one
of its files, it will re-evaluate the file. So you can go ahead and
save the file. As expected it shows `Hello World` in the Neon console.

Let's change the code to:

``` python
# NEON session

for i in range(42):
    log(f'Hello World {i}')
```

Upon save, it evaluates the file and you'll see plenty of hello
worlds. The `live` folder is the primary area in which you work, you
can define your libraries of facilities there, prepare your helper
functions, and so on.

!!! note

    Files are loaded in alphabetical order in the `live` folder, so you
    can take advantage of this and have your helpers, facilities always
    loaded in a file prefixed with `_` for instance.

Now, let's prepare something more useful.
