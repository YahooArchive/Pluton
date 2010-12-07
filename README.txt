Pluton is a simple framework which makes network applications
lighter-weight, easier to debug, lower latency *and* potentially less
resource intensive. The core features of the framework include:

  o an interface which encourages and simplifies parallel requests
    to reduce latency (without the complexity of threads!)

  o a convenient, fast and simple API available in C, C++, php,
    perl and Java on FreeBSD and RHEL.

  o encapsulation of services into self-contained, independent
    executables

  o a serialization-agnostic interface between clients and services -
    you get to choose fast or pretty

Pluton was designed and developed by Mark Delany at Yahoo! and is used
across many parts of the Yahoo network to substantially reduce
web-serving latency and increase resiliency.

--
