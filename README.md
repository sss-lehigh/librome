*UNDER CONSTRUCTION: Proceed with caution*

# Overview

Rome is a collection of useful utilities for research-oriented programming. 
This is the ethos of this project, to accumulate helpful tools that can serve as the basis for development.

Some of the current features are:
* Workload driver library (see `rome/colosseum`) for experimental evaluation
* Data structures (see `rome/internal`)
* Logging utilities (see `rome/logging`)
* Measurements library (see `rome/metrics`)
* Test utilities (see `rome/testutil`)
* Sundry other utilities (see `rome/utilitites`)

The wishlist includes:
* Portability utilities
* Allocation libraries
* RDMA support
* NUMA utilities

As they say, "Rome wasn't built in a day".

# Workload Driver Library (`rome/colosseum`)
In order to test a system, there must be some kind of workload delivered to it.
The focus of this library is to provide an easy to use library to do so.
The first important concept is a `ClientAdaptor`, which represents an entity that translates operations provided by a workload driver into the actual operations that need to be performed on a given client.
Operations are produced by a `Stream`, which is an abstraction wrapping a sequence of outputs that may be infinte, or not.
A `MappedStream` outputs a single stream from one or more input streams.
This is the primary way to generate operations passed to the `ClientAdaptor`.
It is the job of a `WorkloadDriver` to take the operations produced by a `Stream` and apply them to a `ClientAdaptor`.
This process continues until either the `Stream` terminates or there is an error and can be controlled by a `QpsController` that limits the throughput of operations.
Using a `QpsController` is helpful to determine max throughput by ramping up the throughput over time, or to hold the QPS at a constant value to understand behavior under a certain load.

For more details and examples of how to use this portion of the project, take a peek at the code and tests in `rome/colosseum`.
The name of the folder is tounge-in-cheek, but the Colosseum was where gladiators put their abilities to the test so it felt suitable.
