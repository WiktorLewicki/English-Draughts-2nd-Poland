### English Draughts Engine & Bot
[Leaderboard](https://www.codingame.com/multiplayer/bot-programming/checkers/leaderboard)


This project is my implementation of an engine and bot for English draughts. It currently ranks 18th worldwide and 2nd in Poland on CodinGame. Since CodinGame compiles C++ code with the `-O0` flag, every aspect of the implementation is highly optimized, sometimes at the expense of code readability. Please forgive any less-than-ideal code structure.

The core algorithm is based on minimax with a search depth of 8, enhanced by alpha-beta pruning, a transposition table, an evaluation function, and several other heuristics. With a move execution time of just 100ms on the platform, optimization is key.

I am currently refactoring the code to use bitboards, which should enable a search depth of 10 within the same time constraints, and I’m also working on improving the evaluation function.
