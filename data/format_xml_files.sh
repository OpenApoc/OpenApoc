#!/bin/bash

find -iname '*.xml' -exec xmllint --format --encode UTF-8 {} --output {} \;
