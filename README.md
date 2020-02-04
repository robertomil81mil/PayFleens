# PayFleens
PayFleens is an UCI Chess Enginew which uses the alpha-beta framework. PayFleens started as a goal to learn C watching the BlueFeverSoft's YouTube video serie programing Vice ([Video Instructional Chess Engine](https://www.chessprogramming.org/Vice)) and for the moment it's using Vice as codebase for move generation. PayFleens is also greatly inspired by [Ethereal](https://github.com/AndyGrant/Ethereal) and [Stockfish](https://stockfishchess.org/). You can compile PayFleens using gcc or g++ with the makefile inside of the file `src`.

## UCI parameters

Currently, PayFleens has the following UCI options:

* ### Hash
  The size of the hash table in megabytes. For analysis the more hash given the better.

* ### Move Overhead
  Amount of miliseconds used as time delay. This is useful to avoid losses on time
  due to network and GUI overheads.

* ### Minimum Thinking Time
  The minimum number of miliseconds to search per move.

* ### Slow Mover
  Multipletor used in time management. Lower values will make PayFleens take less time in games, higher values will make it think longer.

## Special Thanks
#### Thank you BlueFeverSoft, Andrew Grant and the Stockfish team for all of your help.