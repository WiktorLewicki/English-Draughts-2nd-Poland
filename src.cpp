#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target("rdrnd", "popcnt", "avx2", "bmi2")
#include <bits/stdc++.h>
#include <sys/time.h>
#include <cstring>
using namespace std;

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)

const int8_t MAX_DEPTH = 8;
const int16_t INF1 = 12000;
const int16_t INF2 = 9900;

struct Move {
    int8_t x1, y1, x2, y2;
    Move() {}
    Move(int8_t a, int8_t b, int8_t c, int8_t d)
        : x1(a), y1(b), x2(c), y2(d) {}
};

struct BitsetSet {
    uint64_t bits = 0;

    ALWAYS_INLINE void insert(const pair<int8_t,int8_t>& p) noexcept {
        bits |= (1ULL << ((p.first << 3) | p.second));
    }
    ALWAYS_INLINE void erase(const pair<int8_t,int8_t>& p) noexcept {
        bits &= ~(1ULL << ((p.first << 3) | p.second));
    }
    ALWAYS_INLINE void clear() noexcept { bits = 0; }
    ALWAYS_INLINE int count() const noexcept { return __builtin_popcountll(bits); }

    struct Iterator {
        uint64_t mask;
        Iterator(uint64_t m): mask(m) {}
        pair<int8_t,int8_t> operator*() const {
            int idx = __builtin_ctzll(mask);
            return { int8_t(idx >> 3), int8_t(idx & 7) };
        }
        Iterator& operator++() {
            mask &= mask - 1;
            return *this;
        }
        bool operator!=(const Iterator& o) const { return mask != o.mask; }
    };

    Iterator begin() const noexcept { return { bits }; }
    Iterator end()   const noexcept { return { 0ULL }; }
};


enum Flag { EXACT, LOWERBOUND, UPPERBOUND };

struct TTEntry {
    uint64_t key;
    int8_t depth;
    int16_t value;
    uint8_t flag;
};

constexpr size_t TT_SIZE = 1 << 17;
static TTEntry tt[TT_SIZE];

ALWAYS_INLINE void tt_clear() {
    memset(tt, 0, sizeof(tt));
}

ALWAYS_INLINE TTEntry* tt_lookup(uint64_t key) {
    size_t index = key & (TT_SIZE - 1);
    while (true) {
        if(tt[index].key == key)
            return &tt[index];
        if(tt[index].key == 0)
            return nullptr;
        index = (index + 1) & (TT_SIZE - 1);
    }
}

ALWAYS_INLINE void tt_store(uint64_t key, int8_t depth, int16_t value, uint8_t flag) {
    size_t index = key & (TT_SIZE - 1);
    while(tt[index].key != 0 && tt[index].key != key)
        index = (index + 1) & (TT_SIZE - 1);
    tt[index].key = key;
    tt[index].depth = depth;
    tt[index].value = value;
    tt[index].flag = flag;
}


class Game {
private:
    int it1, it2;
    bool who;
    bool my_color;
    bool attack_is_possible;
    BitsetSet figures[2];
    array<int8_t, 4> setup[20];
    array<int8_t, 5> history[40];
    int8_t board[8][8];
    int8_t last1, last2;

    ALWAYS_INLINE void init() {
        it1 = it2 = -1;
        attack_is_possible = false;
        who = false;
        last1 = last2 = -1;
        figures[0].clear();
        figures[1].clear();
        for (int8_t i = 0; i < 8; ++i)
            for (int j = 0; j < 8; j++)
                board[i][j] = 0;
        for (int8_t i = 0; i < 8; i += 2) {
            board[0][i] = board[2][i] = 1;
            figures[0].insert({0, i});
            figures[0].insert({2, i});
            board[6][i] = 2;
            figures[1].insert({6, i});
        }
        for (int8_t i = 1; i < 8; i += 2) {
            board[5][i] = board[7][i] = 2;
            figures[1].insert({5, i});
            figures[1].insert({7, i});
            board[1][i] = 1;
            figures[0].insert({1, i});
        }
    }

    ALWAYS_INLINE bool has(const int8_t x, const int8_t y, const bool color) const {
        return board[x][y] && (((board[x][y] & 1) ^ color) != 0);
    }

    ALWAYS_INLINE bool gd(const int8_t x, const int8_t y) const {
        return (unsigned)x < 8 && (unsigned)y < 8;
    }

    ALWAYS_INLINE void promote(const int8_t x, const int8_t y) {
        if(board[x][y] < 3) {
            board[x][y] += 2;
            ++it2;
            history[it2] = {x, y, -1, -1, -1};
            last1 = last2 = -2;
        }
    }

    ALWAYS_INLINE void move(const int8_t x1, const int8_t y1, const int8_t x2, const int8_t y2, const bool save_to_history) {
        if(save_to_history) {
            ++it2;
            history[it2] = {x1, y1, x2, y2, 0};
        }
        figures[who].erase({x1, y1});
        board[x2][y2] = board[x1][y1];
        board[x1][y1] = 0;
        figures[who].insert({x2, y2});
        if(x2 == (who ? 0 : 7)) promote(x2, y2);
    }

    ALWAYS_INLINE void attack(const int8_t x1, const int8_t y1, const int8_t x2, const int8_t y2) {
        int8_t p1 = ((x1 + x2) >> 1), p2 = ((y1 + y2) >> 1);
        ++it2;
        history[it2] = {x1, y1, x2, y2, board[p1][p2]};
        move(x1, y1, x2, y2, false);
        figures[!who].erase({p1, p2});
        board[p1][p2] = 0;
    }

public:
    Game(const string &mycolor) {
        my_color = (mycolor == "b");
        init();
    }

    ALWAYS_INLINE uint64_t getHash() const {
        uint64_t hash = 1469598103934665603ULL;
        for (int i = 0; i < 8; i++){
            for (int j = ((i+1)&1); j < 8; j++){
                hash ^= (unsigned char)board[i][j];
                hash *= 1099511628211ULL;
            }
        }
        hash ^= static_cast<uint64_t>(who);
        hash *= 1099511628211ULL;
        hash ^= static_cast<uint64_t>(last1 + 128);
        hash *= 1099511628211ULL;
        hash ^= static_cast<uint64_t>(last2 + 128);
        hash *= 1099511628211ULL;
        hash ^= static_cast<uint64_t>(attack_is_possible);
        hash *= 1099511628211ULL;
        return hash;
    }

    ALWAYS_INLINE void undo() {
        array<int8_t, 4> settings = setup[it1];
        it1--;
        who = settings[0];
        last1 = settings[1];
        last2 = settings[2];
        attack_is_possible = settings[3];
        array<int8_t, 5> last_move = history[it2];
        it2--;
        if(last_move[4] == -1) {
            board[last_move[0]][last_move[1]] -= 2;
            last_move = history[it2];
            it2--;
        }
        figures[who].erase({last_move[2], last_move[3]});
        figures[who].insert({last_move[0], last_move[1]});
        board[last_move[0]][last_move[1]] = board[last_move[2]][last_move[3]];
        board[last_move[2]][last_move[3]] = 0;
        if(last_move[4] > 0) {
            int p1 = ((last_move[0] + last_move[2]) >> 1), p2 = ((last_move[1] + last_move[3]) >> 1);
            board[p1][p2] = last_move[4];
            figures[!who].insert({p1, p2});
        }
    }

    ALWAYS_INLINE int8_t can(const int8_t x1, const int8_t y1, const int8_t x2, const int8_t y2) const {
        if(UNLIKELY(!gd(x1, y1) || !gd(x2, y2))) return 0;
        if(last1 != -1 && (x1 != last1 || y1 != last2)) return 0;
        if(!has(x1, y1, who)) return 0;
        if(board[x2][y2]) return 0;
        int8_t vc = who ? -1 : 1;
        if(!attack_is_possible && x1 + vc == x2 && ((y1 - 1) == y2 || (y1 + 1) == y2))
            return 1;
        if(x1 + (vc << 1) == x2 &&
           ((y1 - 2 == y2 && has(x1 + vc, y1 - 1, !who)) ||
            (y1 + 2 == y2 && has(x1 + vc, y1 + 1, !who))))
            return 2;
        if(board[x1][y1] > 2) {
            vc = -vc;
            if(!attack_is_possible && x1 + vc == x2 && ((y1 - 1) == y2 || (y1 + 1) == y2))
                return 1;
            if(x1 + (vc << 1) == x2 &&
               ((y1 - 2 == y2 && has(x1 + vc, y1 - 1, !who)) ||
                (y1 + 2 == y2 && has(x1 + vc, y1 + 1, !who))))
                return 2;
        }
        return 0;
    }

    ALWAYS_INLINE bool figure_possibly_knock(const int8_t i, const int8_t j, const int8_t vc) const {
        return can(i, j, i + vc, j - vc) || can(i, j, i + vc, j + vc) ||
               (board[i][j] > 2 && (can(i, j, i - vc, j - vc) || can(i, j, i - vc, j + vc)));
    }

    ALWAYS_INLINE bool check_possibly_knock() const {
        int8_t vc = who ? -2 : 2;
        for(auto v : figures[who])
            if(figure_possibly_knock(v.first, v.second, vc))
                return true;
        return false;
    }

    ALWAYS_INLINE int8_t final_move(const int8_t x1, const int8_t y1, const int8_t x2, const int8_t y2) {
        int8_t possible = can(x1, y1, x2, y2);
        if(!possible) return 0;
        ++it1;
        setup[it1] = { (int8_t)who, last1, last2, (int8_t)attack_is_possible };
        if(possible == 1) {
            move(x1, y1, x2, y2, true);
            last1 = last2 = -1;
            who = !who;
            attack_is_possible = check_possibly_knock();
            return 1;
        }
        last1 = x2; last2 = y2;
        attack(x1, y1, x2, y2);
        int8_t vc = who ? -2 : 2;
        if(figure_possibly_knock(x2, y2, vc)) {
            last1 = x2; last2 = y2;
            attack_is_possible = true;
            return 3;
        }
        last1 = last2 = -1;
        who = !who;
        attack_is_possible = check_possibly_knock();
        return 2;
    }

    ALWAYS_INLINE bool kto() const { return who; }
    ALWAYS_INLINE bool get_color() const { return my_color; }
    ALWAYS_INLINE const BitsetSet& getFigures(int player) const noexcept { return figures[player]; }
    ALWAYS_INLINE int8_t get_cell(const int8_t x, const int8_t y) const noexcept { return board[x][y]; }

    ALWAYS_INLINE void reset1() {
        who = !my_color;
        figures[0].clear(); figures[1].clear();
        it1 = it2 = -1;
        last1 = last2 = -1;
    }
    ALWAYS_INLINE void reset2() { attack_is_possible = check_possibly_knock(); }
    ALWAYS_INLINE void ustaw(int k, string &s) {
        for (int j = 0; j < 8; j++) {
            if(s[j] == '.')
                board[k][j] = 0;
            else if(s[j] == 'r' || s[j] == 'R') {
                figures[1].insert({(int8_t)k, (int8_t)j});
                board[k][j] = (s[j] == 'r') ? 2 : 4;
            } else {
                figures[0].insert({(int8_t)k, (int8_t)j});
                board[k][j] = (s[j] == 'b') ? 1 : 3;
            }
        }
    }
};

ALWAYS_INLINE int countAvailableMoves(const Game &g, int8_t x, int8_t y, bool color) {
    int cnt = 0;
    int8_t dir = color ? -1 : 1;
    if(g.can(x, y, x+dir, y-dir)) ++cnt;
    if(g.can(x, y, x+dir, y+dir)) ++cnt;
    if(g.can(x, y, x+2*dir, y-2*dir)) ++cnt;
    if(g.can(x, y, x+2*dir, y+2*dir)) ++cnt;
    if(g.get_cell(x, y) > 2) {
        int8_t ndir = -dir;
        if(g.can(x, y, x+ndir, y-ndir)) ++cnt;
        if(g.can(x, y, x+ndir, y+ndir)) ++cnt;
        if(g.can(x, y, x+2*ndir, y-2*ndir)) ++cnt;
        if(g.can(x, y, x+2*ndir, y+2*ndir)) ++cnt;
    }
    return cnt;
}

int16_t evaluatePosition(const Game &g) {
    constexpr int16_t PAWN_BASE         = 100;
    constexpr int16_t KING_BASE         = 170;
    constexpr int16_t KING_ENDGAME      = 220;
    constexpr int16_t PROMO_BONUS       = 15;
    constexpr int16_t CAPTURE_BONUS     = 20;
    constexpr int16_t MOBILITY_BONUS    = 2;
    constexpr int16_t TRAPPED_PENALTY   = 10;
    constexpr int16_t CENTER_BONUS      = 3;
    constexpr int16_t BACKRANK_BONUS    = 5;
    constexpr int16_t PROTECTION_BONUS  = 5;
    constexpr int16_t ISOLATION_PENALTY = 7;
    constexpr int16_t ADVANCEMENT_BONUS = 4;
    constexpr int16_t CONNECTIVITY_BONUS= 3;
    constexpr int16_t CHAIN_BONUS       = 6;
    constexpr int16_t KING_MOBILITY_BONUS = 3;
    constexpr int16_t VULNERABILITY_PENALTY = 5;
    constexpr int16_t EXCHANGE_MULTIPLIER = 2;

    bool me = g.get_color();

    if (g.getFigures(me).count() == 0)
        return -INF2;
    if (g.getFigures(!me).count() == 0)
        return INF2;

    int totalPieces = g.getFigures(0).count() + g.getFigures(1).count();
    int16_t effectiveKingValue = (totalPieces <= 6 ? KING_ENDGAME : KING_BASE);
    int16_t score = 0;

    static const int8_t centerTable[8][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 2, 2, 2, 2, 1, 0},
        {0, 1, 2, 3, 3, 2, 1, 0},
        {0, 1, 2, 3, 3, 2, 1, 0},
        {0, 1, 2, 2, 2, 2, 1, 0},
        {0, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 0, 0, 0, 0, 0, 0}
    };

    for (int player = 0; player < 2; player++) {
        const BitsetSet &figs = g.getFigures(player);
        int16_t sign = (player == me ? 1 : -1);
        for (auto p : figs) {
            int8_t x = p.first, y = p.second;
            int8_t cell = g.get_cell(x, y);
            bool isKing = (cell > 2);

            int16_t val = isKing ? effectiveKingValue : PAWN_BASE;
            val += centerTable[x][y] * CENTER_BONUS;

            if (!isKing) {
                int8_t advancement = (player == 0 ? (7 - x) : x);
                val += advancement * ADVANCEMENT_BONUS;
            }

            if (!isKing && ((player == 0 && x == 0) || (player == 1 && x == 7))) {
                val += BACKRANK_BONUS;
            }

            if (!isKing) {
                if ((player == 0 && x <= 1) || (player == 1 && x >= 6))
                    val += PROMO_BONUS;
            }

            int movesCount = countAvailableMoves(g, x, y, player);
            val += (movesCount == 0 ? -TRAPPED_PENALTY : movesCount * MOBILITY_BONUS);

            if (g.can(x, y, x+2, y+2) || g.can(x, y, x+2, y-2) ||
                g.can(x, y, x-2, y+2) || g.can(x, y, x-2, y-2))
                val += CAPTURE_BONUS;

            int protectionCount = 0;
            const int dx[4] = {-1, -1, 1, 1};
            const int dy[4] = {-1, 1, -1, 1};
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8 && g.get_cell(nx, ny) != 0) {
                    bool sameOwner = (((g.get_cell(nx, ny) & 1) ^ player) == 0);
                    if (sameOwner)
                        protectionCount++;
                }
            }
            val += protectionCount * PROTECTION_BONUS;
            if (protectionCount == 0)
                val -= ISOLATION_PENALTY;

            int connectivity = 0;
            const int dx2[4] = {-2, -2, 2, 2};
            const int dy2[4] = {-2, 2, -2, 2};
            for (int d = 0; d < 4; d++) {
                int nx = x + dx2[d], ny = y + dy2[d];
                if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8 && g.get_cell(nx, ny) != 0) {
                    bool sameOwner = (((g.get_cell(nx, ny) & 1) ^ player) == 0);
                    if (sameOwner)
                        connectivity++;
                }
            }
            val += connectivity * CONNECTIVITY_BONUS;
            if (!isKing) {
                int bx = (player == 0 ? x - 1 : x + 1);
                if (bx >= 0 && bx < 8) {
                    if (y - 1 >= 0 && g.get_cell(bx, y - 1) != 0 &&
                        (((g.get_cell(bx, y - 1) & 1) ^ player) == 0))
                        val += CHAIN_BONUS;
                    if (y + 1 < 8 && g.get_cell(bx, y + 1) != 0 &&
                        (((g.get_cell(bx, y + 1) & 1) ^ player) == 0))
                        val += CHAIN_BONUS;
                }
            }

            if (isKing) {
                int kingMoves = countAvailableMoves(g, x, y, player);
                val += kingMoves * KING_MOBILITY_BONUS;
                if (x == 0 || x == 7 || y == 0 || y == 7)
                    val -= KING_MOBILITY_BONUS;
            }
            if (connectivity == 0)
                val -= VULNERABILITY_PENALTY;

            score += sign * val;
        }
    }
    int16_t myCount = g.getFigures(!me).count();
    int16_t oppCount = g.getFigures(me).count();
    if(score <= -450 || myCount - oppCount >= 4 ){
        int16_t bonus = (myCount - oppCount) * 180;
        score -= bonus;
    }
    else if(score >= 450 || oppCount - myCount >=4){
        int16_t bonus = (oppCount - myCount) * 180;
        score += bonus;
    }
    return score;
}


ALWAYS_INLINE int scoreMove(const Game &g, const Move &m) {
    int score = 0;
    int8_t piece = g.get_cell(m.x1, m.y1);
    if (ABS(m.x1 - m.x2) == 2) score += 100;
    if (piece < 3 && m.x2 == (g.kto() ? 7 : 0)) score += 50;
    if (piece < 3) score += 10;
    if (m.x2 >= 2 && m.x2 <= 5 && m.y2 >= 2 && m.y2 <= 5) score += 5;
    return score;
}

ALWAYS_INLINE void sortMoves(const Game &g, Move moves[], int moveCount) {
    for (int i = 1; i < moveCount; i++) {
        Move key = moves[i];
        int keyScore = scoreMove(g, key);
        int j = i - 1;
        while(j >= 0 && scoreMove(g, moves[j]) < keyScore) {
            moves[j+1] = moves[j];
            j--;
        }
        moves[j+1] = key;
    }
}

static struct timeval global_start_time;

int16_t minimax(Game &g, int8_t depth, int16_t alpha, int16_t beta, bool knt) {
    struct timeval now;
    gettimeofday(&now, NULL);
    long elapsed = (now.tv_sec - global_start_time.tv_sec) * 1000 + (now.tv_usec - global_start_time.tv_usec) / 1000;
    if (elapsed >= 100)
        return evaluatePosition(g);
    
    uint64_t key = g.getHash();
    TTEntry* entry = tt_lookup(key);
    if(entry != nullptr && entry->depth >= depth) {
         if(entry->flag == EXACT)
              return entry->value;
         if(entry->flag == LOWERBOUND)
              alpha = MAX(alpha, entry->value);
         if(entry->flag == UPPERBOUND)
              beta = MIN(beta, entry->value);
         if(alpha >= beta)
              return entry->value;
    }
    
    if(depth <= 0 && !knt) {
        int16_t eval = evaluatePosition(g);
        tt_store(key, depth, eval, EXACT);
        return eval;
    }
    
    Move moves[50];
    int moveCount = 0;
    bool turn = g.kto();
    int8_t vc1 = turn ? -1 : 1, vc2 = vc1 * 2;
    for(auto p : g.getFigures(turn)) {
        int8_t x = p.first, y = p.second;
        if(g.can(x, y, x+vc1, y-vc1))
            moves[moveCount++] = Move(x, y, x+vc1, y-vc1);
        if(g.can(x, y, x+vc1, y+vc1))
            moves[moveCount++] = Move(x, y, x+vc1, y+vc1);
        if(g.can(x, y, x+vc2, y-vc2))
            moves[moveCount++] = Move(x, y, x+vc2, y-vc2);
        if(g.can(x, y, x+vc2, y+vc2))
            moves[moveCount++] = Move(x, y, x+vc2, y+vc2);
        if(g.get_cell(x, y) > 2) {
            int8_t kvc1 = -vc1, kvc2 = -vc2;
            if(g.can(x, y, x+kvc1, y-kvc1))
                moves[moveCount++] = Move(x, y, x+kvc1, y-kvc1);
            if(g.can(x, y, x+kvc1, y+kvc1))
                moves[moveCount++] = Move(x, y, x+kvc1, y+kvc1);
            if(g.can(x, y, x+kvc2, y-kvc2))
                moves[moveCount++] = Move(x, y, x+kvc2, y-kvc2);
            if(g.can(x, y, x+kvc2, y+kvc2))
                moves[moveCount++] = Move(x, y, x+kvc2, y+kvc2);
        }
    }
    if(moveCount == 0) {
        int16_t result = (g.kto() == g.get_color()) ? -(INF2 + depth) : (INF2 + depth);
        tt_store(key, depth, result, EXACT);
        return result;
    }
    sortMoves(g, moves, moveCount);
    
    bool maximizing = (g.kto() == g.get_color());
    int16_t best = maximizing ? -INF1 : INF1;
    int16_t origAlpha = alpha, origBeta = beta;
    for(int i = 0; i < moveCount; i++) {
        bool ct = (g.final_move(moves[i].x1, moves[i].y1, moves[i].x2, moves[i].y2) == 3);
        int16_t score = minimax(g, depth - 1, alpha, beta, ct);
        g.undo();
        if(maximizing) {
            best = MAX(best, score);
            alpha = MAX(alpha, best);
        } else {
            best = MIN(best, score);
            beta = MIN(beta, best);
        }
        if(alpha >= beta)
            break;
        
        gettimeofday(&now, NULL);
        elapsed = (now.tv_sec - global_start_time.tv_sec) * 1000 + (now.tv_usec - global_start_time.tv_usec) / 1000;
        if (elapsed >= 100)
            break;
    }
    
    uint8_t flag;
    if(best <= origAlpha)
        flag = UPPERBOUND;
    else if(best >= origBeta)
        flag = LOWERBOUND;
    else
        flag = EXACT;
    tt_store(key, depth, best, flag);
    return best;
}

Move findBestMove(Game &g) {
    Move moves[50];
    int moveCount = 0;
    bool turn = g.kto();
    int8_t vc1 = turn ? -1 : 1, vc2 = vc1 * 2;
    for(auto p : g.getFigures(turn)) {
        int8_t x = p.first, y = p.second;
        if(g.can(x, y, x+vc1, y-vc1))
            moves[moveCount++] = Move(x, y, x+vc1, y-vc1);
        if(g.can(x, y, x+vc1, y+vc1))
            moves[moveCount++] = Move(x, y, x+vc1, y+vc1);
        if(g.can(x, y, x+vc2, y-vc2))
            moves[moveCount++] = Move(x, y, x+vc2, y-vc2);
        if(g.can(x, y, x+vc2, y+vc2))
            moves[moveCount++] = Move(x, y, x+vc2, y+vc2);
        if(g.get_cell(x, y) > 2) {
            int8_t kvc1 = -vc1, kvc2 = -vc2;
            if(g.can(x, y, x+kvc1, y-kvc1))
                moves[moveCount++] = Move(x, y, x+kvc1, y-kvc1);
            if(g.can(x, y, x+kvc1, y+kvc1))
                moves[moveCount++] = Move(x, y, x+kvc1, y+kvc1);
            if(g.can(x, y, x+kvc2, y-kvc2))
                moves[moveCount++] = Move(x, y, x+kvc2, y-kvc2);
            if(g.can(x, y, x+kvc2, y+kvc2))
                moves[moveCount++] = Move(x, y, x+kvc2, y+kvc2);
        }
    }
    tt_clear();
    sortMoves(g, moves, moveCount);
    Move bestMove = moves[0];
    int16_t bestScore = INF1;
    for(int i = 0; i < moveCount; i++) {
        g.final_move(moves[i].x1, moves[i].y1, moves[i].x2, moves[i].y2);
        int16_t score = minimax(g, MAX_DEPTH - 1, -INF1, INF1, false);
        g.undo();
        if(score < bestScore) {
            bestScore = score;
            bestMove = moves[i];
        }
    }
    return bestMove;
}

ALWAYS_INLINE string translate(const Move ruch) {
    string res;
    res.push_back('A' + ruch.y1);
    res.push_back('1' + ruch.x1);
    res.push_back('A' + ruch.y2);
    res.push_back('1' + ruch.x2);
    return res;
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string my_color;
    getline(cin, my_color);
    Game gra(my_color);
    while(1) {
        
        gra.reset1();
        for(int i = 0; i < 8; i++) {
            string input_line;
            getline(cin, input_line);
            gra.ustaw(7 - i, input_line);
        }
        gra.reset2();
        int legal_moves;
        cin >> legal_moves; cin.ignore();
        for(int i = 0; i < legal_moves; i++) {
            string move_string;
            cin >> move_string; cin.ignore();
        }
        string ans;
        int info;
        struct timeval start, end;
        gettimeofday(&start, NULL);
        global_start_time = start;
        do {
            Move ruch = findBestMove(gra);
            info = gra.final_move(ruch.x1, ruch.y1, ruch.x2, ruch.y2);
            if(ans.empty())
                ans = translate(ruch);
            else
                ans += translate(ruch).substr(2,2);
        } while(info == 3);
        cout << ans << "\n";
        
        gettimeofday(&end, NULL);
        long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
        cerr << ms << "\n";
    }
}
