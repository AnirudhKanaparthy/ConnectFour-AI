# Connect Four AI

This repository features two algorithms tailored for adversarial gaming:

1. Minimax Algorithm (enhanced with alpha-beta pruning)
2. Monte Carlo Tree Search

## Key Insights:

1. Board representation utilizes dual bitsets, hence `ConnectN::Game` and `ConnectN::Player` are templates. Input the board dimensions `<rows, cols>` accordingly.
2. There are two tile types: `Positive` and `Negative`. Each game includes precisely two players, one per tile type. Note: A game cannot have players with identical tile types.
3. For extensive `depth` in Minimax or large `simulation` numbers in Monte Carlo, employing the `-O3` flag is advisable.
4. Compile with `-std=c++2a` flag.

## Implementation Guide:

The `main.cpp` file exemplifies usage. Nonetheless, here are the steps:

1. Initialize the random number generator:

```cpp
srand(time(0));
```

2. Instantiate players:

   - Human Player:

   ```cpp
   ConnectN::HumanPlayer<S_ROWS, S_COLS> playerHuman("Human Identifier", ConnectN::Tile::Positive);
   ```

   - Minimax AI Agent (requires specifying AI's tile, then the opponent's):

   ```cpp
   ConnectN::MinimaxPlayer playerMinimax(max_depth, "Mrs. Minimax", ConnectN::Tile::Negative, ConnectN::Tile::Positive);
   ```

   - Monte Carlo Tree Search AI Agent:

   ```cpp
   ConnectN::MonteCarloPlayer<S_ROWS, S_COLS> playerMonteCarlo(number_of_simulations, UCT_constant, "Mr. Monte Carlo", ConnectN::Tile::Negative);
   ```

3. Create a `ConnectN::Game` instance with both players:

```cpp
ConnectN::Game game{ConnectN::Game(playerPositive, playerNegative)};
```

4. Initiate the game loop:

```cpp
game.gameLoop();
```

### Example Integration:

```cpp
int main() {
    constexpr int rows{6};
    constexpr int cols{7};
    srand(time(0));

    const float c{1.5};
    const int simulations{150000};

    ConnectN::HumanPlayer<rows, cols> playerHuman("Human", ConnectN::Tile::Positive);
    ConnectN::MonteCarloPlayer<rows, cols> playerMonteCarlo(simulations, c, "Mr. Monte Carlo", ConnectN::Tile::Negative);

    ConnectN::Game game{ConnectN::Game(playerHuman, playerMonteCarlo)};
    game.gameLoop();
}
```

## Observations and Insights:

1. Optimal depth is 7 for a balance between speed and strategy. Higher depths yield diminishing returns.
2. Ideal UCT constant: 1.5 or \(\sqrt{2}\).
   - Above 1.5, excessive exploration occurs at the expense of defense.
   - Below this threshold, the focus narrows, exploiting immediate winning moves without adequate defense.
3. Recommended simulations: 100,000 for time efficiency. 200,000 outperforms Minimax at depths 4 or 5.

## Future Directions:

Exploring larger games like Chess, Shogi, Checkers, or Go is next, as Connect Four's limited branching factor makes Monte Carlo less effective compared to Minimax. No future updates for this project are planned, but the learnings here will fuel larger-scale game development.
