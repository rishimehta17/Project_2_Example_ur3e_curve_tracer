# Sphinx config for the Read the Docs site. The only page is the repo README.
project = 'UR3e Curve Tracer'
extensions = ['myst_parser']         # Markdown support
myst_heading_anchors = 3             # GitHub-style #section links work here too
html_theme = 'sphinx_rtd_theme'
exclude_patterns = ['_build']
