#!/usr/bin/python

import sys
# import content_filter

from raven import Client
import logging

logger = logging.getLogger("sentry.errors")
handler = logging.StreamHandler()
formatter = logging.Formatter("[%(levelname)s] %(name)s: %(message)s")
handler.setFormatter(formatter)
logger.addHandler(handler)

dsn = 'https://566c01c61e764a3a85acee7e67342179:7478983997a14667b8f0d36dfa32a7aa@sentry.fond.io/34' #default: test

client = Client(dsn)

# filename = sys.argv[1]
message = ' '.join(sys.argv[1:])


content = ''.join([line for line in sys.stdin.read()])

if content.strip() == '':
    sys.exit(0)
elif len(sys.argv) == 4 and sys.argv[3] == 'diff':
    f = open(sys.argv[1], "r")
    temp_file = f.read()
    f.close()
    f = open(sys.argv[2], "w")
    f.write(temp_file)
    f.close()

# print content

client.capture('raven.events.Message', message=message, data={
    'request': {
        'url'  : '/sentry/report',
        'data' : content,
        'query_string': '',
        'method': 'PUSH',
    }
})