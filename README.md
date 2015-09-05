# CARS-System

This C project implements a good non-graphical car sharing system in a client-server fashion.

## Overview

The system implements two kinds of processes:

+  **docars**: the client process (one for each user connected to the system) that interacts with the server to send car sharing offers/requests related to the connected user;
+  **mgcars**: the server process that verifies if an user is authorized to send car sharing offers/requests,
keep track of car sharing requests and offers and send answers to clients.

Both *docars* and *mgcars* are multithreading processes and they communicates via AF_UNIX sockets.
Furthermore, the system provides a bash script named **carstat** that analyzes a *log file* written by the server and that contains some car sharing *statistics* relative to offers and requests.

## Setup and Execution

TODO

## Usage example

TODO
