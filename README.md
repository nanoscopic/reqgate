# ReqGate
Request Gate - Minimal HTTP server designed to translate between HTTP protocol and nanomsg xml messages

This project is meant to replace the combination of Apache and modnano. Its purpose is to accept HTTP requests,
convert them to nanomsg xml messages, send those on to the Lumith ECM system, have them handled and return a nanomsg
xml response, take that response and render it back to the client via HTTP protocol.

The project is simplistic intentionally so as to make it easier to develop. It is also in a mid-development state.
As-is without modifications it probably won't do anything that you want it to do. A number of things remain to be done,
including using message queueing internally with multiple worker threads so that the entire system works in an event
based fashion rather than needing to have a thread per open request. Because it is not yet threaded or event based,
the performance right now will only be marginally better than Apache with modnano.
