#!/bin/sh

set -e
typst compile specification.typ specification.pdf
git add specification.pdf