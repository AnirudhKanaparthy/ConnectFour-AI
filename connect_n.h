#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

namespace ConnectN {
struct Shape {
    int rows;
    int cols;

    inline int n_elems() { return rows * cols; }
};

struct Vec2i {
    int x;
    int y;

    bool operator==(const Vec2i& other) const {
        return (x == other.x) && (y == other.y);
    }

    Vec2i operator+(Vec2i& vec) { return {x + vec.x, y + vec.y}; }
    Vec2i operator+(Vec2i vec) { return {x + vec.x, y + vec.y}; }
    Vec2i operator-(Vec2i& vec) { return {x - vec.x, y - vec.y}; }
    Vec2i operator-(Vec2i vec) { return {x - vec.x, y - vec.y}; }
    Vec2i operator-() { return {-x, -y}; }

    Vec2i operator*(Vec2i& vec) { return {x * vec.x, y * vec.y}; }
    Vec2i operator*(Vec2i vec) { return {x * vec.x, y * vec.y}; }
    Vec2i operator*(int c) { return {x * c, y * c}; }

    Vec2i operator/(Vec2i& vec) { return {x / vec.x, y / vec.y}; }
    Vec2i operator/(Vec2i vec) { return {x / vec.x, y / vec.y}; }
};

enum class Tile : int8_t {
    Negative = -1,
    Empty = 0,
    Positive = 1,
};
Tile getEnemyTile(Tile t) {
    switch (t) {
        case Tile::Negative:
            return Tile::Positive;
        case Tile::Positive:
            return Tile::Negative;
        default:
            return Tile::Empty;
    }
}

std::ostream& operator<<(std::ostream& os, Tile tile) {
    switch (tile) {
        case Tile::Negative:
            os << "O";
            break;
        case Tile::Positive:
            os << "X";
            break;
        case Tile::Empty:
            os << ".";
            break;
        default:
            throw std::invalid_argument("Invalid Tile argument");
    }
    return os;
}

struct Move {
    Vec2i pos;
    Tile tile;

    bool operator==(const Move& other) const {
        return (pos == other.pos) && (tile == other.tile);
    }
};

}  // namespace ConnectN

template <>
struct std::hash<ConnectN::Vec2i> {
    std::size_t operator()(const ConnectN::Vec2i& vec) const {
        return std::hash<int>()(vec.x) ^ std::hash<int>()(vec.y);
    }
};

template <>
struct std::hash<ConnectN::Move> {
    std::size_t operator()(const ConnectN::Move& move) const {
        return std::hash<ConnectN::Vec2i>()(move.pos) ^
               std::hash<int>()(static_cast<int>(move.tile));
    }
};

namespace ConnectN {

template <size_t S_ROWS, size_t S_COLS>
class Board {
    const int connectN{4};
    Shape m_shape{S_ROWS, S_COLS};
    std::bitset<S_ROWS * S_COLS> m_positivePieces;
    std::bitset<S_ROWS * S_COLS> m_negativePieces;

   private:
    bool isValidPosition(Vec2i& pos) const {
        return !(pos.x < 0 || pos.x >= m_shape.cols || pos.y < 0 ||
                 pos.y >= m_shape.rows);
    }
    inline int index(Vec2i& pos) const { return pos.y * m_shape.cols + pos.x; }

   public:
    Board() : m_positivePieces(0), m_negativePieces(0) {}
    Shape shape() const { return m_shape; }
    const int N() const { return connectN; }

    std::optional<Tile> operator[](Vec2i pos) const {
        if (!isValidPosition(pos)) {
            return {};
        }
        std::bitset<S_ROWS * S_COLS> mask{1ULL};

        if (((m_positivePieces >> index(pos)) & mask) == mask) {
            return Tile::Positive;
        } else if (((m_negativePieces >> index(pos)) & mask) == mask) {
            return Tile::Negative;
        }
        return Tile::Empty;
    }

    bool operator<<(Move move) {
        if (!isValidPosition(move.pos)) {
            return false;
        }

        Vec2i below{move.pos.x, move.pos.y + 1};
        if (move.pos.y + 1 != m_shape.rows && (*this)[below] == Tile::Empty) {
            return false;
        }

        std::bitset<S_ROWS * S_COLS> mask{1ULL};
        mask = mask << index(move.pos);

        switch (move.tile) {
            case Tile::Positive:
                m_positivePieces = m_positivePieces | mask;
                break;
            case Tile::Negative:
                m_negativePieces = m_negativePieces | mask;
                break;
            default:
                return false;
        }

        return true;
    }

    bool operator>>(Move move) {
        if (!isValidPosition(move.pos)) {
            return false;
        }
        std::bitset<S_ROWS * S_COLS> mask{1ULL};
        mask = ~(mask << index(move.pos));

        switch (move.tile) {
            case Tile::Positive:
                m_positivePieces = m_positivePieces & mask;
                break;
            case Tile::Negative:
                m_negativePieces = m_negativePieces & mask;
                break;
            default:
                return false;
        }
        return true;
    }
};

template <size_t S_ROWS, size_t S_COLS>
std::ostream& operator<<(std::ostream& os, Board<S_ROWS, S_COLS>& board) {
    Shape boardShape{board.shape()};

    std::string full_horizontal_line =
        "+" + std::string((boardShape.cols + 2) * 3, '-') + "+";
    std::string inner_horizontal_line =
        "+---" + std::string((boardShape.cols - 1) * 4, '-') + "+";

    os << full_horizontal_line << "\n";

    for (int y{0}; y < boardShape.rows; ++y) {
        os << "| ";
        for (int x{0}; x < boardShape.cols; ++x) {
            std::optional<Tile> tile{board[{x, y}]};
            if (!tile) {
                throw std::exception();
            }
            os << tile.value();
            os << " | ";
        }
        os << "\n";

        if (y != boardShape.rows - 1) {
            os << inner_horizontal_line << "\n";
        } else {
            os << full_horizontal_line << "\n";
        }
    }

    return os;
}

template <size_t S_ROWS, size_t S_COLS>
std::pair<std::optional<long>, long> evaluate(Board<S_ROWS, S_COLS>& board) {
    const int N{board.N()};

    std::array<Vec2i, 4> directions{{{1, 0}, {0, 1}, {1, -1}, {1, 1}}};
    Shape boardShape{board.shape()};

    std::function<int(Vec2i, Vec2i, Tile)> check{
        [&board, &boardShape, &N](Vec2i p, Vec2i d, Tile lastTile) -> int {
            int count{0};
            for (int i = 1; i < N; ++i) {
                Vec2i current{p + d * i};
                if (current.x < 0 || current.x >= boardShape.cols ||
                    current.y < 0 || current.y >= boardShape.rows) {
                    break;
                }

                auto curOpt{board[current]};
                if (!curOpt || curOpt.value() != lastTile) {
                    break;
                }
                count++;
            }
            return count;
        }};

    long score{0};
    for (int y{0}; y < boardShape.rows; ++y) {
        for (int x{0}; x < boardShape.cols; ++x) {
            auto tOpt{board[{x, y}]};
            if (!tOpt || tOpt.value() == Tile::Empty) {
                continue;
            }

            Vec2i p{x, y};
            Tile lastTile{tOpt.value()};
            for (auto d : directions) {
                int count = 1;  // count the last placed token
                count += check(p, d, lastTile);
                if (count >= N) {
                    return {static_cast<long>(lastTile),
                            (lastTile == Tile::Positive
                                 ? std::numeric_limits<long>::max()
                                 : std::numeric_limits<long>::min())};
                }
                count += check(p, -d, lastTile);
                if (count >= N) {
                    return {static_cast<long>(lastTile),
                            (lastTile == Tile::Positive
                                 ? std::numeric_limits<long>::max()
                                 : std::numeric_limits<long>::min())};
                }

                score += std::pow(10, count) * static_cast<long>(lastTile);
            }
        }
    }
    // Draw Check
    bool isDraw{true};
    for (int i{0}; i < boardShape.cols; ++i) {
        if (board[{i, 0}] == Tile::Empty) {
            isDraw = false;
            break;
        }
    }

    if (isDraw) {
        return {0, 0};
    }

    return {{}, score};
}

template <size_t S_ROWS, size_t S_COLS>
std::pair<std::optional<long>, long> evaluate(Board<S_ROWS, S_COLS>& board,
                                              Vec2i lastPosition) {
    const int N{board.N()};

    std::array<Vec2i, 4> directions{{{1, 0}, {0, 1}, {1, -1}, {1, 1}}};
    Shape boardShape{board.shape()};

    std::function<int(Vec2i, Tile)> check{
        [&board, &boardShape, &N, &lastPosition](Vec2i d,
                                                 Tile lastTile) -> int {
            int count{0};
            for (int i = 1; i < N; ++i) {
                Vec2i current{lastPosition + d * i};
                if (current.x < 0 || current.x >= boardShape.cols ||
                    current.y < 0 || current.y >= boardShape.rows) {
                    break;
                }

                auto curOpt{board[current]};
                if (!curOpt || curOpt.value() != lastTile) {
                    break;
                }
                count++;
            }
            return count;
        }};

    auto tOpt{board[lastPosition]};
    if (!tOpt || tOpt.value() == Tile::Empty) {
        return {};
    }

    long score{0};
    Tile lastTile{tOpt.value()};
    for (auto d : directions) {
        int count = 1;  // count the last placed token
        count += check(d, lastTile);
        if (count >= N) {
            return {static_cast<long>(lastTile),
                    (lastTile == Tile::Positive
                         ? std::numeric_limits<long>::max()
                         : std::numeric_limits<long>::min())};
        }
        count += check(-d, lastTile);
        if (count >= N) {
            return {static_cast<long>(lastTile),
                    (lastTile == Tile::Positive
                         ? std::numeric_limits<long>::max()
                         : std::numeric_limits<long>::min())};
        }

        score += std::pow(10, count) * static_cast<long>(lastTile);
    }
    // Draw Check
    bool isDraw{true};
    for (int i{0}; i < boardShape.cols; ++i) {
        if (board[{i, 0}] == Tile::Empty) {
            isDraw = false;
            break;
        }
    }

    if (isDraw) {
        return {0, 0};
    }

    return {{}, score};
}

template <size_t S_ROWS, size_t S_COLS>
std::vector<Vec2i> generateValidPositions(const Board<S_ROWS, S_COLS>& board) {
    Shape boardShape{board.shape()};
    std::vector<Vec2i> res;
    res.reserve(boardShape.cols);

    for (int x{0}; x < boardShape.cols; ++x) {
        if (board[{x, 0}] != Tile::Empty) {
            continue;
        }
        for (int level{1}; level < boardShape.rows; ++level) {
            if (board[{x, level}] != Tile::Empty) {
                res.push_back({x, level - 1});
                break;
            }
        }
        if (res.back().x != x) {
            res.push_back({x, boardShape.rows - 1});
        }
    }

    return res;
}

template <size_t S_ROWS, size_t S_COLS>
class Player {
   public:
    virtual std::string_view getFriendlyName() = 0;
    virtual Tile getPlayerTile() = 0;
    virtual Move getNextMove(Board<S_ROWS, S_COLS>& board) = 0;
};

template <size_t S_ROWS, size_t S_COLS>
class Game {
   private:
    Board<S_ROWS, S_COLS> board;
    bool isGameOver;

    Player<S_ROWS, S_COLS>* playerPositive;
    Player<S_ROWS, S_COLS>* playerNegative;

    Player<S_ROWS, S_COLS>* currentPlayer;

   private:
    void swapPlayers() {
        if (currentPlayer == playerPositive) {
            currentPlayer = playerNegative;
        } else if (currentPlayer == playerNegative) {
            currentPlayer = playerPositive;
        } else {
            throw std::exception();
        }
    }

   public:
    Game(Player<S_ROWS, S_COLS>* t_playerPositive,
         Player<S_ROWS, S_COLS>* t_playerNegative)
        : board(),
          isGameOver(false),
          playerPositive(t_playerPositive),
          playerNegative(t_playerNegative),
          currentPlayer(t_playerPositive) {}

    std::optional<long> makeMove() {
        Move newMove{currentPlayer->getNextMove(board)};
        if (!isGameOver && board << newMove) {
            swapPlayers();
            return evaluate(board, newMove.pos).first;
        } else {
            return evaluate(board).first;
        }
    }

    void gameLoop() {
        const int maxLimit{1000};
        for (int i{0}; i < maxLimit; ++i) {
            std::cout << board << "\n";

            std::cout << "It is " << currentPlayer->getFriendlyName()
                      << "'s Turn\n";
            std::optional<long> valueOpt{makeMove()};
            if (valueOpt) {
                long value{valueOpt.value()};
                switch (value) {
                    case +1:
                        std::cout << "Player "
                                  << playerPositive->getFriendlyName()
                                  << " has won!\n";
                        break;
                    case 0:
                        std::cout << "It's a draw!\n";
                        break;
                    case -1:
                        std::cout << "Player "
                                  << playerNegative->getFriendlyName()
                                  << " has won!\n";
                        break;
                    default:
                        throw std::exception();
                }
                isGameOver = true;
                break;
            }
        }
        std::cout << board << "\n\n";
    }
};
}  // namespace ConnectN