#!/bin/bash

find -name '*.xml' -exec xmllint --format --encode UTF-8 {} --output {} \;
