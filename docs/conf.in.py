# ----------------------------------------------------------------------------
# -                        Open3D: www.open3d.org                            -
# ----------------------------------------------------------------------------
# Copyright (c) 2018-2023 www.open3d.org
# SPDX-License-Identifier: MIT
# ----------------------------------------------------------------------------

# -*- coding: utf-8 -*-
#
# Open3D documentation build configuration file, created by
# sphinx-quickstart on Mon Apr  3 14:18:28 2017.
#
# This file is execfile()d with the current directory set to its
# containing dir.
#
# Note that not all possible configuration values are present in this
# autogenerated file.
#
# All configuration values have a default; values that are commented out
# serve to show the default.

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

import os
import re
import subprocess
import sys
from typing import List


def get_git_short_hash():
    try:
        rc = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"])
        rc = rc.decode("utf-8").strip()
        return rc
    except subprocess.CalledProcessError:
        return "unknown"


# Import open3d raw python package with the highest priority
# This is a trick to show open3d.open3d as open3d in the docs
# Only tested to work on Unix
current_file_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(
    0,
    os.path.join(current_file_dir, "..", "build", "lib", "python_package",
                 "open3d"),
)

# -- General configuration ------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinx.ext.mathjax",
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.napoleon",
    "sphinx.ext.todo",
    "nbsphinx",
    'm2r2',
]

if os.environ["skip_notebooks"] == "true":
    print("Skipping Jupyter notebooks")
    extensions = [e for e in extensions if e != "nbsphinx"]

# Allow for more time for notebook cell evaluation
nbsphinx_timeout = 6000
# We pre-execute nbs. Some (jupyter_visualizer.ipynb) shouldn't be executed
nbsphinx_execute = 'never'
# nbsphinx_allow_errors = True

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
source_suffix = ['.rst', '.md']

# The master toctree document.
master_doc = "index"

# General information about the project.
project = u"Open3D"
copyright = u"2018 - 2021, www.open3d.org"
author = u"www.open3d.org"

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.

# This value can be overwritten in make_docs.py when sphinx-build is called.
# Usually, the `version` value is set to the current git commit hash.
# At Open3D releases, the `version` value is set to Open3D version number.
current_hash = get_git_short_hash()
version = "master ({})".format(current_hash)
release = version

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = "en"

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This patterns also effect to html_static_path and html_extra_path
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "**.ipynb_checkpoints",
    "docker.in.rst",
    "getting_started.in.rst",
    "jupyter/*/*.ipynb",
    "python_api_in/*.rst",
]

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = "sphinx"

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = False

# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
# html_theme = 'alabaster'
theme_path = "@OPEN3D_SPHINX_THEME_SOURCE_DIR@"
html_theme = "sphinx_rtd_theme"
html_theme_path = [theme_path]
html_favicon = "_static/open3d_logo.ico"

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#
html_theme_options = {
    # 'display_version': True
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".

# '_static' contains the theme overwrite
static_path = os.path.join(theme_path, "sphinx_rtd_theme", "static")
html_static_path = [static_path, "_static"]

# Force table wrap: https://rackerlabs.github.io/docs-rackspace/tools/rtd-tables.html
html_context = {
    "css_files": [
        "_static/theme_overrides.css"  # override wide tables in RTD theme
    ]
}

# added by Jaesik to hide "View page source"
html_show_sourcelink = False

# -- Options for HTMLHelp output ------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = "Open3Ddoc"

# -- Options for LaTeX output ---------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    #
    # 'papersize': 'letterpaper',
    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',
    # Additional stuff for the LaTeX preamble.
    #
    # 'preamble': '',
    # Latex figure (float) alignment
    #
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [(
    master_doc,
    "Open3D.tex",
    u"Open3D Documentation",
    u"Qianyi Zhou",
    "manual",
)]

# -- Options for manual page output ---------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [(master_doc, "open3d", u"Open3D Documentation", [author], 1)]

# -- Options for Texinfo output -------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [(
    master_doc,
    "Open3D",
    u"Open3D Documentation",
    author,
    "Open3D",
    "One line description of project.",
    "Miscellaneous",
)]

# Version 0: Added by Jaesik to list Python members using the source order
# Version 1: Changed to 'groupwise': __init__ first, then methods, then
#            properties. Within each, sorted alphabetically.
autodoc_member_order = "groupwise"

# Show TODO elements in the documentation
todo_include_todos = True


def is_enum_class(func, func_name):

    def import_from_str(class_name):
        components = class_name.split(".")
        mod = __import__(components[0])
        for comp in components[1:]:
            mod = getattr(mod, comp)
        return mod

    is_enum = False
    try:
        if func_name == "name" and "self: handle" in func.__doc__:
            is_enum = True
        else:
            pattern = re.escape(func_name) + r"\(self: ([a-zA-Z0-9_\.]*).*\)"
            m = re.match(pattern, func.__doc__)
            if m:
                c_name = m.groups()[0]
                c = import_from_str(c_name)
                if hasattr(c, "__entries"):
                    is_enum = True
    except:
        pass
    return is_enum


# Keep the __init__ function doc
def skip(app, what, name, obj, would_skip, options):
    if name in {"__init__", "name"}:
        if is_enum_class(obj, name):
            return True
        else:
            return False
    return would_skip


ESCAPE_VERTICAL_BAR_IN_CLASSES_DOCS: List[str] = [
    "open3d.data.RedwoodIndoorLivingRoom1",
    "open3d.data.RedwoodIndoorLivingRoom2",
    "open3d.data.RedwoodIndoorOffice1",
    "open3d.data.RedwoodIndoorOffice2",
]


def escape_vertical_bars(app, what, name, obj, options, lines: List[str]):
    # Note - Latex docs also contain Vertical Bar, so we
    # apply this filtering only to specific classes.
    if name in ESCAPE_VERTICAL_BAR_IN_CLASSES_DOCS:
        for index in range(len(lines)):
            lines[index] = lines[index].replace("|", "\|")


def setup(app):
    app.connect("autodoc-skip-member", skip)
    app.connect("autodoc-process-docstring", escape_vertical_bars)
    # Add Google analytics
    app.add_js_file("https://www.googletagmanager.com/gtag/js?id=G-3TQPKGV6Z3",
                    **{'async': 'async'})
    app.add_js_file(None,
                    body="""
        window.dataLayer = window.dataLayer || [];
        function gtag(){dataLayer.push(arguments);}
        gtag('js', new Date());
        gtag('config', 'G-3TQPKGV6Z3');""")
