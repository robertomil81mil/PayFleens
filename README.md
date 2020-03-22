# PayFleens
PayFleens is a UCI Chess Engine which uses the alpha-beta framework. PayFleens started as a goal to learn C with the BlueFeverSoft's YouTube video serie programing Vice. ([Video Instructional Chess Engine](https://www.chessprogramming.org/Vice)) At the moment PayFleens is using Vice as codebase for move generation. PayFleens is also greatly inspired by [Ethereal](https://github.com/AndyGrant/Ethereal) and [Stockfish](https://stockfishchess.org/). You can compile PayFleens with the `makefile` inside of `src` (default compliler: gcc).

#### Thank you BlueFeverSoft, Andrew Grant and the Stockfish team for all of your help.

## UCI parameters

Currently, PayFleens has the following UCI options:

* ### Hash
  The size of the hash table in megabytes. For analysis the more hash given the better.

* ### Move Overhead
  Amount of miliseconds used as time delay. This is useful to avoid losses on time
  due to network and GUI overheads.

* ### Minimum Thinking Time
  Minimum number of miliseconds to search per move.

* ### Slow Mover
  Multipletor used in time management. Lower values will make PayFleens take less time in games, higher values will make it think longer.
