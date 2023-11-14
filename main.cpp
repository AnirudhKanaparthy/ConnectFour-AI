#include <iostream>

#include "connect_n.h"

namespace ConnectN {

template <size_t S_ROWS, size_t S_COLS>
class HumanPlayer : public Player<S_ROWS, S_COLS> {
   public:
    HumanPlayer(std::string_view t_name, Tile t_tile)
        : m_name(t_name), m_tile(t_tile) {}

    std::string_view getFriendlyName() override { return m_name; }
    Tile getPlayerTile() override { return m_tile; }
    Move getNextMove(Board<S_ROWS, S_COLS>& board) override {
        Vec2i p{};
        std::cout << "What will be your move " << m_name << "?\n";
        std::cin >> p.x;

        Shape boardShape{board.shape()};
        p.y = boardShape.rows - 1;
        for (int i{0}; i < boardShape.rows; ++i) {
            if (board[{p.x, i}] != Tile::Empty) {
                p.y = i - 1;
                break;
            }
        }

        return {p, m_tile};
    }

   private:
    std::string m_name;
    Tile m_tile;
};

template <size_t S_ROWS, size_t S_COLS>
class MinimaxPlayer : public Player<S_ROWS, S_COLS> {
   public:
    MinimaxPlayer(int t_depth, std::string_view t_name, Tile t_tile,
                  Tile t_enemyTile)
        : m_depth(t_depth),
          m_name(t_name),
          m_tile(t_tile),
          m_enemyTile(t_enemyTile) {}

    std::string_view getFriendlyName() override { return m_name; }
    Tile getPlayerTile() override { return m_tile; }

    std::pair<long, Move> alphabeta(Board<S_ROWS, S_COLS>& board, long alpha,
                                    long beta, long depth, bool isEnemy,
                                    Move& lastMove) {
        auto [isTerminal, score]{evaluate(board)};
        if (depth == 0 || isTerminal) {
            return {score, {}};
        }

        Tile currentTile{!isEnemy ? m_tile : m_enemyTile};

        bool isMaximising{(m_tile == Tile::Positive)
                              ? (!isEnemy ? true : false)
                              : (!isEnemy ? false : true)};

        std::vector<Vec2i> positions;
        positions.reserve(board.shape().cols);
        positions = generateValidPositions(board);

        Move resultMove{positions[0], currentTile};
        if (isMaximising) {
            // Is a maximising player
            long maxValue = std::numeric_limits<long>::min();
            for (Vec2i& p : positions) {
                Move m{p, currentTile};
                board << m;

                std::pair<long, Move> res{
                    alphabeta(board, alpha, beta, depth - 1, !isEnemy, m)};
                board >> m;

                if (res.first > maxValue) {
                    maxValue = res.first;
                    resultMove = m;
                }
                alpha = std::max(alpha, res.first);
                if (beta <= alpha) {
                    break;
                }
            }
            return {maxValue, resultMove};
        } else {
            // Is a minimising player
            long minValue = std::numeric_limits<long>::max();
            for (auto& p : positions) {
                Move m{p, currentTile};
                board << m;

                std::pair<long, Move> res{
                    alphabeta(board, alpha, beta, depth - 1, !isEnemy, m)};
                board >> m;

                if (res.first < minValue) {
                    minValue = res.first;
                    resultMove = m;
                }
                beta = std::min(beta, res.first);
                if (beta <= alpha) {
                    break;
                }
            }
            return {minValue, resultMove};
        }
    }

    Move getNextMove(Board<S_ROWS, S_COLS>& board) override {
        Board<S_ROWS, S_COLS> newBoard(board);
        Move dummy{};
        auto [_, res]{alphabeta(newBoard, std::numeric_limits<long>::min(),
                                std::numeric_limits<long>::max(), m_depth,
                                false, dummy)};

        return res;
    }

   private:
    int m_depth;
    std::string m_name;
    Tile m_tile;
    Tile m_enemyTile;
};

template <size_t S_ROWS, size_t S_COLS>
class MonteCarloNode {
   public:
    int visits;
    int wins;

   public:
    MonteCarloNode(Tile t_turn, Board<S_ROWS, S_COLS>& t_board)
        : m_isTerminal(false),
          m_winState(0),
          m_turn(t_turn),
          m_isFullyExpanded(false),
          visits(0),
          wins(0),
          m_board(t_board),
          m_parent(nullptr) {
        m_maxChildren = generateValidPositions(m_board).size();
    }

    ~MonteCarloNode() {
        for (auto child : children) {
            delete child.second;
        }
    }

    bool isTerminal() {
        if (!m_isTerminal) {
            auto [terminal, score]{evaluate(m_board)};
            m_isTerminal = !!terminal;

            if (terminal) {
                m_winState = terminal.value();
            }
        }
        return m_isTerminal;
    }

    bool isFullyExpanded() {
        if (!m_isFullyExpanded && m_maxChildren == children.size()) {
            m_isFullyExpanded = true;
        }
        return m_isFullyExpanded;
    }

    bool applyMove(Move move) {
        if (m_board << move) {
            m_maxChildren = generateValidPositions(m_board).size();

            isTerminal();
            isFullyExpanded();
            m_turn = getEnemyTile(m_turn);

            return true;
        }
        return false;
    }

    MonteCarloNode* createChild(Move move) {
        MonteCarloNode* newNode{new MonteCarloNode(m_turn, m_board)};
        newNode->applyMove(move);
        newNode->m_parent = this;
        children[move] = newNode;

        return newNode;
    }

    std::vector<Move> getExpandedMoves() {
        std::vector<Move> moves;
        moves.reserve(children.size());
        for (auto [move, child] : children) {
            moves.push_back(move);
        }
        return moves;
    }

    std::optional<MonteCarloNode*> getChildForMove(Move move) {
        if (children.find(move) == children.end()) {
            return {};
        }
        return children[move];
    }

    const Board<S_ROWS, S_COLS>& getBoard() { return m_board; }

    Tile getTurn() { return m_turn; }

    MonteCarloNode* getParent() { return m_parent; }

    std::optional<long> getWinState() {
        isTerminal();
        return m_winState;
    }

   private:
    Board<S_ROWS, S_COLS> m_board;

    // A Win, Lose, Draw situation
    bool m_isTerminal;
    long m_winState;

    Tile m_turn;
    int m_maxChildren;
    // If all the moves which can be applied to the node have been applied. In
    // other words the number of moves you can make in this board position equal
    // to the number oof children it has.
    bool m_isFullyExpanded;
    MonteCarloNode* m_parent;
    std::unordered_map<Move, MonteCarloNode*> children;
};

template <size_t S_ROWS, size_t S_COLS>
class MonteCarloPlayer : public Player<S_ROWS, S_COLS> {
   public:
    MonteCarloPlayer(int t_nSimulations, int t_c, std::string_view t_name,
                     Tile t_tile)
        : m_nSimulation(t_nSimulations),
          m_c(t_c),
          m_name(t_name),
          m_tile(t_tile) {}

   public:
    std::string_view getFriendlyName() override { return m_name; }
    Tile getPlayerTile() override { return m_tile; }

    std::pair<Move, MonteCarloNode<S_ROWS, S_COLS>*> bestUCT(
        MonteCarloNode<S_ROWS, S_COLS>* node) {
        float maxVal{-std::numeric_limits<float>::max()};
        std::pair<Move, MonteCarloNode<S_ROWS, S_COLS>*> res{};

        std::vector<Move> allMoves{node->getExpandedMoves()};

        for (auto move : allMoves) {
            MonteCarloNode<S_ROWS, S_COLS>* child{
                node->getChildForMove(move)
                    .value()};  // Here a check isn't neccesary because we
                                // are accessing the already existing
                                // children.

            float cq{static_cast<float>(child->wins)};
            float cn{static_cast<float>(child->visits)};

            float rn{static_cast<float>(node->visits)};
            float val{cq / cn + m_c * std::sqrt(std::log(rn) / cn)};

            if (val > maxVal) {
                maxVal = val;
                res = {move, child};
            }
        }

        return res;
    }

    MonteCarloNode<S_ROWS, S_COLS>* expand(
        MonteCarloNode<S_ROWS, S_COLS>* node) {
        auto positions{generateValidPositions(node->getBoard())};

        for (auto pos : positions) {
            auto res{node->getChildForMove({pos, node->getTurn()})};
            if (!res) {
                return node->createChild({pos, node->getTurn()});
            }
        }

        return nullptr;
    }

    MonteCarloNode<S_ROWS, S_COLS>* traverse(
        MonteCarloNode<S_ROWS, S_COLS>* node) {
        std::pair<Move, MonteCarloNode<S_ROWS, S_COLS>*> res{};

        while (!node->isTerminal()) {
            if (!node->isFullyExpanded()) {
                return expand(node);
            }
            res = bestUCT(node);
            node = res.second;
        }
        return res.second;
    }

    Move playoutPolicy(MonteCarloNode<S_ROWS, S_COLS>* node) {
        auto moves{generateValidPositions(node->getBoard())};
        int idx = rand() % moves.size() + 0;

        return {moves[idx], node->getTurn()};
    }

    std::optional<long> playout(MonteCarloNode<S_ROWS, S_COLS>* node) {
        if (node->isTerminal()) {
            return node->getWinState();
        }

        MonteCarloNode<S_ROWS, S_COLS>* simNode{new MonteCarloNode(*node)};
        while (!simNode->isTerminal()) {
            Move move{playoutPolicy(simNode)};
            if (!simNode->applyMove(move)) {
                throw std::exception();
            }
        }
        std::optional<long> res{simNode->getWinState()};

        delete simNode;
        return res;
    }

    void backpropagate(MonteCarloNode<S_ROWS, S_COLS>* root,
                       long playoutResults) {
        MonteCarloNode<S_ROWS, S_COLS>* node{root};

        while (!(node->getParent() == nullptr)) {
            node->wins += (playoutResults * static_cast<int>(m_tile));
            node->visits++;
            node = node->getParent();
        }

        // Again to update root node
        node->wins += (playoutResults * static_cast<int>(m_tile));
        node->visits++;
    }

    Move monteCarloTreeSearch(Board<S_ROWS, S_COLS>& board) {
        MonteCarloNode<S_ROWS, S_COLS>* root{new MonteCarloNode(m_tile, board)};

        MonteCarloNode<S_ROWS, S_COLS>* leaf;

        for (int i{0}; i < m_nSimulation; ++i) {
            leaf = traverse(root);
            auto results{playout(leaf)};
            backpropagate(leaf, results.value());
        }

        std::vector<Move> allMoves{root->getExpandedMoves()};
        Move res{};
        int maxVisits{0};
        for (auto move : allMoves) {
            MonteCarloNode<S_ROWS, S_COLS>* child{
                root->getChildForMove(move).value()};
            if (child->visits > maxVisits) {
                maxVisits = child->visits;
                res = move;
            }
        }
        delete root;
        return res;
    }

    Move getNextMove(Board<S_ROWS, S_COLS>& board) override {
        return monteCarloTreeSearch(board);
    }

   private:
    int m_c;
    int m_nSimulation;
    Tile m_tile;
    std::string m_name;
};

}  // namespace ConnectN

template <size_t S_ROWS, size_t S_COLS>
void playTwoPlayers(ConnectN::Player<S_ROWS, S_COLS>* playerPositive,
                    ConnectN::Player<S_ROWS, S_COLS>* playerNegative) {
    ConnectN::Game game{ConnectN::Game(playerPositive, playerNegative)};
    game.gameLoop();
}

template <size_t S_ROWS, size_t S_COLS>
void humanVsMonteCarlo(int simulations, int c) {
    ConnectN::HumanPlayer<S_ROWS, S_COLS> playerHuman("Human",
                                                      ConnectN::Tile::Positive);
    ConnectN::MonteCarloPlayer<S_ROWS, S_COLS> playerMonteCarlo(
        simulations, c, "Mr. Monte Carlo", ConnectN::Tile::Negative);

    playTwoPlayers(&playerHuman, &playerMonteCarlo);
}

template <size_t S_ROWS, size_t S_COLS>
void humanVsMinimax(int depth) {
    ConnectN::HumanPlayer playerHuman("Human", ConnectN::Tile::Positive);

    ConnectN::MinimaxPlayer playerMinimax(depth, "Mrs. Minimax",
                                          ConnectN::Tile::Negative,
                                          ConnectN::Tile::Positive);
    playTwoPlayers(&playerHuman, &playerMinimax);
}
template <size_t S_ROWS, size_t S_COLS>
void minimaxVsMonteCarlo(int depth, int simulations, float c) {
    ConnectN::MinimaxPlayer<S_ROWS, S_COLS> playerMinimax(
        depth, "Mrs. Minimax", ConnectN::Tile::Positive,
        ConnectN::Tile::Negative);
    ConnectN::MonteCarloPlayer<S_ROWS, S_COLS> playerMonteCarlo(
        simulations, c, "Mr. Monte Carlo", ConnectN::Tile::Negative);

    playTwoPlayers(&playerMinimax, &playerMonteCarlo);
}

template <size_t S_ROWS, size_t S_COLS>
void minimaxVsMinimax(int depthA, int depthB) {
    ConnectN::MinimaxPlayer<S_ROWS, S_COLS> playerMinimaxA(
        depthA, "Mrs. Minimax A", ConnectN::Tile::Positive,
        ConnectN::Tile::Negative);
    ConnectN::MinimaxPlayer<S_ROWS, S_COLS> playerMinimaxB(
        depthB, "Mrs. Minimax B", ConnectN::Tile::Negative,
        ConnectN::Tile::Positive);

    playTwoPlayers(&playerMinimaxA, &playerMinimaxB);
}

int main() {
    constexpr int rows{6};
    constexpr int cols{7};
    srand(time(0));

    // const float c{std::sqrt(2)};  // should be atleast std::sqrt(2)
    const float c{1.5};  // This seems to be the best apparently.

    const int minimaxDepth{7};
    const int monteCarloSimulations{150000};

    // minimaxVsMinimax<rows, cols>(7, 7);  // 7 seems to be the max limit. We
    // get dimishing returns after this.

    // minimaxVsMonteCarlo<rows, cols>(minimaxDepth, monteCarloSimulations, c);
    humanVsMonteCarlo<rows, cols>(monteCarloSimulations, c);
    // humanVsMinimax<rows, cols>(minimaxDepth);

    return 0;
}