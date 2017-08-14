#!/bin/bash

pwd=`pwd`

diff $1 $2 | /usr/local/bin/sentry_report.py $1 $2 diff


