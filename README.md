# marvin

## Purpose

Marvin is a stub for real-time clients that:

* Consume recent and real-time user behavior logs (think tail -f).
* Keep track of user behavior and user engagement data on the fly (think a fixed set of simple data analysis queries).
* Augment log messages with real-time feature values and perhaps their labels (gather labeled data).
* Generate personalized recommendations (based on models trained on the data gathered).

Marvin is written in C++ and uses Thrift as an external interface.

The original design is to pipe in the output of ```tail -f``` or ```fetch --last_ms_and_follow``` from ```npm install overlog``` as the input and use ```npm install ariadne``` as the frontend.

## Motivation

![marvin](https://raw2.github.com/dkorolev/marvin/master/static/marvin.png)

User engagement. Don't talk to me about user engagement.

## Usage

Would require boost and thrift installed. For installation instructions, check out the README of this repository: https://github.com/dkorolev/sandbox_node_cpp_server

TODO(dkorolev): Put a README into this repository.
