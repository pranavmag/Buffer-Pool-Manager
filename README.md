# Buffer Pool Manager

A small C++ implementation of a database buffer pool manager, built from scratch as a learning project focused on understanding **database internals, concurrency, and systems programming**.

## Overview

Databases don't normally read and write every page directly from disk whenever data is accessed. Instead, they maintain a region of memory called a **buffer pool** that caches frequently accessed database pages.

This project implements a simplified version of that system.

The `BufferPoolManager` is responsible for moving pages between the simulated disk and memory, tracking which pages are currently cached, managing page pinning and dirty state, synchronizing concurrent access, and choosing pages to evict when the buffer pool is full.

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
                           |
                           v
                  reader/writer latch
```

### Main Concepts

* **Page** — A fixed-size unit of database data (4096 bytes). Pages also contain a reader-writer latch for coordinating concurrent access to page contents.

* **Frame** — A stable slot in the buffer pool containing a page and metadata such as its pin count and dirty state.

* **Page Table** — Maps logical page IDs to the frames currently containing them.

* **CLOCK Replacer** — Chooses an unpinned frame to evict when the buffer pool is full.

* **Disk Manager** — Simulates persistent storage and handles reading and writing pages.

* **Buffer Pool Manager** — Coordinates page fetching, pinning, unpinning, eviction, flushing, and synchronization of shared buffer-pool metadata.

## Concurrency Model

The current implementation supports concurrent access using two levels of synchronization.

The `BufferPoolManager` uses a coarse-grained mutex to protect shared metadata such as the page table, frame state, pin counts, dirty flags, frame reassignment, and CLOCK replacement state.

Each `Page` also owns a `std::shared_mutex`, allowing:

* Multiple concurrent readers
* A single exclusive writer
* Readers and writers to synchronize access to page contents independently of buffer-pool metadata

Page latches use RAII through `std::shared_lock` and `std::unique_lock`.

The current lock-ordering rule is:

```text
BufferPoolManager mutex
        ↓
Page latch
```

Callers release page latches before calling operations such as `UnpinPage()`.

## Why I Built This

This project is primarily a **learning exercise**.

The goal is to go beyond learning database concepts theoretically and actually implement some of the mechanisms that real database systems rely on.

The project is intentionally being built incrementally, with correctness and understanding taking priority over immediately building a production-quality database component.

## Current Status

**Buffer Pool Manager v2 — Complete**

The current implementation supports:

* Page fetching
* Page insertion into the buffer pool
* Page pinning and unpinning
* Dirty-page tracking
* CLOCK-based eviction
* CLOCK reference-bit updates
* Dirty-page write-back during eviction
* Explicit page flushing
* Flushing all resident dirty pages
* Simulated disk storage
* Stable frame ownership
* Concurrent buffer-pool access
* Reader-writer page latching
* Thread-safe pin count and dirty-state transitions
* Concurrent eviction
* Concurrent dirty-page eviction and reload

Testing currently includes:

* Single-threaded integration tests
* Concurrent fetch tests
* Concurrent write tests
* Concurrent eviction tests
* Concurrent dirty-eviction tests
* Randomized multi-threaded stress tests
* ThreadSanitizer runs for detecting data races

## Planning

Visit my Obsidian Github repo if you are interested in looking through my notes and planning for this project: https://github.com/pranavmag/Obsidian/tree/main/Database%20Systems/Projects/Buffer%20Pool

## Future Work

Planned improvements include:

* More complete page creation/deletion semantics
* Improved error handling
* Page guard abstractions for safer pin/latch lifetime management
* Finer-grained locking if performance becomes a concern
* More adversarial and randomized concurrency testing
* Heap-file and page-layout implementation
* Hash indexes and B+ trees
* External sorting
* Join algorithms
* Query execution operators
* Basic query optimization

## Project Goal

The long-term goal is to use this project as a foundation for exploring more of the internals behind database systems and to develop stronger **C++ systems programming skills through implementation rather than purely theoretical study**.

The buffer pool is intended to serve as the storage foundation for future components such as heap files, indexes, external sorting, joins, execution operators, and query optimization.
