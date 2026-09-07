# Buffer Pool Manager

A small C++ implementation of a database buffer pool manager, built from scratch as a learning project focused on understanding **database internals and systems programming**.

## Overview

Databases don't normally read and write every page directly from disk whenever data is accessed. Instead, they maintain a region of memory called a **buffer pool** that caches frequently accessed database pages.

This project implements a simplified version of that system.

The `BufferPoolManager` is responsible for moving pages between the simulated disk and memory, tracking which pages are currently cached, managing page pinning and dirty state, and choosing pages to evict when the buffer pool is full.

The simulated disk is represented by an in-memory byte array, allowing the project to focus on buffer management without depending on an actual filesystem or database storage engine.

## Architecture

At a high level:

```text
                    BufferPoolManager
                           |
          +----------------+----------------+
          |                |                |
          v                v                v
      PageTable       ClockReplacer     DiskManager
          |                |                |
          v                v                v
   page_id -> frame   selects victim    simulated disk
                           |
                           v
                         Frame
                           |
                           v
                         Page
```

### Main Concepts

* **Page** — A fixed-size unit of database data (4096 bytes).
* **Frame** — A slot in the buffer pool containing a page and metadata such as its pin count and dirty state.
* **Page Table** — Maps logical page IDs to the frames currently containing them.
* **CLOCK Replacer** — Chooses an unpinned frame to evict when the buffer pool is full.
* **Disk Manager** — Simulates persistent storage and handles reading and writing pages.
* **Buffer Pool Manager** — Coordinates everything and provides operations such as fetching, unpinning, eviction, and flushing pages.

## Why I Built This

This project is primarily a **learning exercise**.

The goal is to go beyond learning database concepts theoretically and actually implement some of the mechanisms that real database systems rely on.

The project is intentionally being built incrementally, with correctness and understanding taking priority over immediately building a production-quality database component.

## Current Status

**Buffer Pool Manager v1 — Complete**

The current implementation supports:

* Page fetching
* Page insertion into the buffer pool
* Page pinning and unpinning
* Dirty-page tracking
* CLOCK-based eviction
* Dirty-page write-back during eviction
* Explicit page flushing
* Flushing all resident dirty pages
* Simulated disk storage

Basic integration tests currently cover fetching, modification, eviction, persistence, and flushing.

## Future Work

Planned improvements include:

* More comprehensive/adversarial testing
* More complete page creation/deletion semantics
* Improved error handling
* CLOCK reference-bit updates
* Concurrency and latching
* Stress testing
* Further database internals built on top of the buffer pool

## Project Goal

The long-term goal is to use this project as a foundation for exploring more of the internals behind database systems and to develop stronger **C++ systems programming skills through implementation rather than purely theoretical study**.
