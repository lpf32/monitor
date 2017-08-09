#!/bin/bash

pwd=`pwd`

diff $1 $2 | $pwd/sentry_report.py $1 $2 diff


