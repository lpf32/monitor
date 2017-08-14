#!/bin/bash

pwd=`pwd`

diff $1 $2 | /home/zhang/CLionProjects/monitor/sentry_report.py $1 $2 diff


