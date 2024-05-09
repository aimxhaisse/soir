from mkdocs.__main__ import cli


cli([
    'build',
    '--site-dir', 'site/',
    '--config-file', 'www/mkdocs.yml'
])
