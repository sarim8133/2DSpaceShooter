#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <random>
#include <memory>

// --- Game Configuration Struct ---

struct GameConfig {
    static constexpr int PLAYER_MAX_HEALTH = 100;

    static constexpr int BOSS_MAX_HEALTH = 120;

    static constexpr int ENEMY_DAMAGE_LEVEL1 = 10;
    static constexpr int ENEMY_DAMAGE_LEVEL2 = 20;
    static constexpr int ENEMY_DAMAGE_DEFAULT = 20;

    static constexpr int BOSS_BULLET_DAMAGE = 30;

    static constexpr float ENEMY_SPEED_LEVEL1 = 0.25f;
    static constexpr float ENEMY_SPEED_LEVEL2 = 0.33f;
    static constexpr float ENEMY_SPAWN_RATE_LEVEL1 = 1.5f;
    static constexpr float ENEMY_SPAWN_RATE_LEVEL2 = 1.1f;
    static constexpr float ENEMY_SPAWN_RATE_DEFAULT = 1000.f;

    static constexpr int LEVEL2_SCORE_THRESHOLD = 80;
    static constexpr int LEVEL3_SCORE_THRESHOLD = 140;
    static constexpr int LEVEL2_MAX_ENEMIES = 4;
    static constexpr int LEVEL2_DOUBLE_SPAWN_FREQ = 5;
    static constexpr float LEVEL2_DOUBLE_SPAWN_SPEED_BONUS = 0.03f;

    static constexpr float BOSS_MOVE_SPEED = 0.5f;
    static constexpr float BOSS_DESCEND_Y = 100.f;
    static constexpr float BOSS_DESCEND_SPEED = 0.2f;
    static constexpr float BOSS_BULLET_SPEED = 0.5f;
    static constexpr float BOSS_BULLET_SPEED_PHASE2 = 1.0f;
    static constexpr float BOSS_BULLET_SPEED_PHASE3 = 1.2f;
    static constexpr float BOSS_ATTACK_INTERVAL = 1.f;
    static constexpr float BOSS_ATTACK_INTERVAL_PHASE2 = 0.7f;
    static constexpr float BOSS_ATTACK_INTERVAL_PHASE3 = 0.5f;
    static constexpr int BOSS_BULLET_DAMAGE_AMOUNT = 30;
    static constexpr int BOSS_HIT_DAMAGE = 10;
    static constexpr int BOSS_PHASE2_HP = 80;
    static constexpr int BOSS_PHASE3_HP = 50;
    static constexpr float BOSS_SPREAD_BULLET_INTERVAL = 3.0f;
    static constexpr float BOSS_MINION_SPAWN_INTERVAL = 3.0f;
    static constexpr int BOSS_SPREAD_BULLET_COUNT = 4;
    static constexpr float BOSS_SPREAD_BULLET_ANGLE = 40.f;

    static constexpr float BULLET_ANIMATION_INTERVAL_MS = 40.f;
    static constexpr float BULLET_MOVE_SPEED = -1.f;
    static constexpr int BULLET_OFFSCREEN_Y = 0;

    static constexpr float EXPLOSION_ANIMATION_INTERVAL_MS = 80.f;
    static constexpr int EXPLOSION_FRAMES = 8;

    static constexpr float HEALTH_BAR_WIDTH = 200.f;
    static constexpr float HEALTH_BAR_HEIGHT = 20.f;
    static constexpr float SHAKE_INTENSITY = 10.f;
    static constexpr float SHAKE_DURATION = 0.3f;

    static constexpr int WINDOW_WIDTH = 800;
    static constexpr int WINDOW_HEIGHT = 600;

    // Powerup config
    static constexpr float HEALTH_POWERUP_SPEED = 0.25f;
    static constexpr float HEALTH_POWERUP_SPAWN_MIN = 8.0f; // seconds
    static constexpr float HEALTH_POWERUP_SPAWN_MAX = 15.0f; // seconds
    static constexpr int HEALTH_POWERUP_AMOUNT = 50;
    static constexpr int HEALTH_POWERUP_MAX = 100;
};

// Helper functions 
std::vector<int> readScoresFromFile(const std::string& filename) {
    std::vector<int> scores;
    std::ifstream infile(filename);
    int s;
    while (infile >> s) {
        scores.push_back(s);
    }
    return scores;
}
void appendScoreToFile(const std::string& filename, int score) {
    std::ofstream outfile(filename, std::ios::app);
    if (outfile.is_open()) {
        outfile << score << std::endl;
    }
}
int getHighScore(const std::string& filename) {
    std::vector<int> scores = readScoresFromFile(filename);
    if (!scores.empty()) {
        return *std::max_element(scores.begin(), scores.end());
    }
    return 0;
}

// --- Game Entities  ---
class Bullet {
public:
    std::vector<sf::Texture> frames;
    sf::Sprite sprite;
    int currentFrame = 0;
    sf::Clock frameClock;
    sf::Vector2f velocity = { 0.f, GameConfig::BULLET_MOVE_SPEED };
    Bullet(const std::vector<sf::Texture>& bulletFrames, sf::Vector2f position) {
        frames = bulletFrames;
        sprite.setTexture(frames[0]);
        sprite.setPosition(position);
    }
    Bullet(const std::vector<sf::Texture>& bulletFrames, sf::Vector2f position, sf::Vector2f vel) {
        frames = bulletFrames;
        sprite.setTexture(frames[0]);
        sprite.setPosition(position);
        velocity = vel;
    }
    void update() {
        if (frameClock.getElapsedTime().asMilliseconds() > GameConfig::BULLET_ANIMATION_INTERVAL_MS) {
            currentFrame = (currentFrame + 1) % frames.size();
            sprite.setTexture(frames[currentFrame]);
            frameClock.restart();
        }
        sprite.move(velocity);
    }
};

class Enemy {
public:
    sf::Sprite sprite;
    float speed;
    Enemy(sf::Texture& texture, sf::Vector2f position) {
        sprite.setTexture(texture);
        sprite.setPosition(position);
        speed = GameConfig::ENEMY_SPEED_LEVEL1;
    }
    virtual void update() { sprite.move(0.f, speed); }
    virtual ~Enemy() {}
};

class AnimatedEnemy : public Enemy {
public:
    std::vector<sf::Texture> frames;
    int currentFrame = 0;
    sf::Clock frameClock;
    AnimatedEnemy(const std::vector<sf::Texture>& enemyFrames, sf::Vector2f position, float spd)
        : Enemy(const_cast<sf::Texture&>(enemyFrames[0]), position), frames(enemyFrames) {
        speed = spd;
        sprite.setTexture(frames[0]);
    }
    void update() override {
        if (frameClock.getElapsedTime().asMilliseconds() > 100) {
            currentFrame = (currentFrame + 1) % frames.size();
            sprite.setTexture(frames[currentFrame]);
            frameClock.restart();
        }
        sprite.move(0.f, speed);
    }
};

class Explosion {
public:
    std::vector<sf::Texture> frames;
    sf::Sprite sprite;
    int currentFrame = 0;
    sf::Clock frameClock;
    bool finished = false;
    Explosion(const sf::Vector2f& position) {
        for (int i = 0; i < GameConfig::EXPLOSION_FRAMES; ++i) {
            sf::Texture tex;
            tex.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/explotion/sprite_" + std::to_string(i) + ".png");
            frames.push_back(tex);
        }
        sprite.setTexture(frames[0]);
        sprite.setPosition(position);
    }
    void update() {
        if (frameClock.getElapsedTime().asMilliseconds() > GameConfig::EXPLOSION_ANIMATION_INTERVAL_MS) {
            currentFrame++;
            if (currentFrame < frames.size()) {
                sprite.setTexture(frames[currentFrame]);
                frameClock.restart();
            }
            else {
                finished = true;
            }
        }
    }
};

class Boss {
public:
    std::vector<sf::Texture> frames;
    sf::Sprite sprite;
    int currentFrame = 0;
    sf::Clock animationClock;
    float moveSpeed = GameConfig::BOSS_MOVE_SPEED;
    int direction = 1;
    int health = GameConfig::BOSS_MAX_HEALTH;
    enum Phase { PHASE1, PHASE2, PHASE3 };
    Phase phase = PHASE1;
    Boss() {
        for (int i = 0; i < 6; ++i) {
            sf::Texture tex;
            tex.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/final_boss_animation/sprite_" + std::to_string(i) + ".png");
            frames.push_back(tex);
        }
        sprite.setTexture(frames[0]);
        sprite.setPosition(300.f, -100.f);
    }
    void update() {
        if (animationClock.getElapsedTime().asMilliseconds() > 100) {
            currentFrame = (currentFrame + 1) % frames.size();
            sprite.setTexture(frames[currentFrame]);
            animationClock.restart();
        }
        sprite.move(moveSpeed * direction, 0.f);
        if (sprite.getPosition().x <= 0 || sprite.getPosition().x + sprite.getGlobalBounds().width >= GameConfig::WINDOW_WIDTH)
            direction *= -1;
    }
    void updatePhase() {
        if (health <= GameConfig::BOSS_PHASE3_HP)
            phase = PHASE3;
        else if (health <= GameConfig::BOSS_PHASE2_HP)
            phase = PHASE2;
        else
            phase = PHASE1;
    }
};

// --- Health PowerUp Entity ---
class HealthPowerUp {
public:
    sf::Sprite sprite;
    float speed;
    bool active;
    HealthPowerUp(const sf::Texture& tex, sf::Vector2f pos, float spd)
        : speed(spd), active(true)
    {
        sprite.setTexture(tex);
        sprite.setPosition(pos);
    }
    void update() {
        sprite.move(0.f, speed);
        if (sprite.getPosition().y > GameConfig::WINDOW_HEIGHT)
            active = false;
    }
};

// --- OOP Game Class ---
class SpaceInvadersGame {
public:
    enum GameState { START, PLAYING, GAME_OVER, YOU_WON, YOU_LOSE, LEVEL_TRANSITION };

    SpaceInvadersGame()
        : window(sf::VideoMode(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT), "Simple Space Shooter"),
        gameState(START),
        scoreFile("scores.txt"),
        fontSizeTitle(24), fontSizeMain(16), fontSizeSmall(10),
        health(GameConfig::PLAYER_MAX_HEALTH), score(0), level(1),
        bossSpawned(false), bossDefeated(false),
        isShaking(false), shakeDuration(0.f), shakeIntensity(GameConfig::SHAKE_INTENSITY),
        level1CompleteSoundPlayed(false), level2CompleteSoundPlayed(false),
        inLevelTransition(false), nextLevel(1), bossMusicStarted(false),
        level2EnemySpeed(GameConfig::ENEMY_SPEED_LEVEL2), level2SpawnRate(GameConfig::ENEMY_SPAWN_RATE_LEVEL2),
        level2DoubleSpawnCounter(0), level2MaxEnemies(GameConfig::LEVEL2_MAX_ENEMIES),
        timeSurvived(0.f), totalKills(0),
        healthPowerUpTimer(0.f), healthPowerUpNextSpawn(0.f)
    {
        loadResources();
        setupUI();
        player.setPosition(400.f, 500.f);
        scheduleNextHealthPowerUp();
    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            update();
            render();
        }
        cleanup();
    }

private:
    // Window and state
    sf::RenderWindow window;
    GameState gameState;
    const std::string scoreFile;

    // Fonts and UI
    sf::Font font;
    const unsigned int fontSizeTitle, fontSizeMain, fontSizeSmall;
    sf::Text startText, winText, loseText, scoreText, levelText;
    sf::Text healthLabelText, healthValueText;
    sf::RectangleShape healthBarBack, healthBarFront;
    sf::Text bossHealthLabelText, bossHealthValueText;
    sf::RectangleShape bossHealthBarBack, bossHealthBarFront;
    sf::Text highScoreText, transitionText;

    // Game variables
    int health, score, level;
    bool bossSpawned, bossDefeated;
    int bossHealth;
    int highScore;

    // Sounds
    sf::SoundBuffer bgBuffer, shootBuffer, explosionBuffer, winBuffer, loseBuffer, levelUpBuffer, levelCompleteBuffer, bossBuffer, healthPowerUpBuffer;
    sf::Sound backgroundSound, shootSound, explosionSound, winSound, loseSound, levelUpSound, levelCompleteSound, bossSound, healthPowerUpSound;

    // Textures
    sf::Texture playerTex, enemyTex, bossTex;
    std::vector<sf::Texture> bulletFrames;
    std::vector<sf::Texture> level2EnemyFrames;
    sf::Texture healthPowerUpTex;

    // Entities
    sf::Sprite player;
    std::vector<Enemy*> enemies;
    std::vector<Bullet> bullets;
    std::vector<Explosion> explosions;
    Boss boss;
    std::vector<Bullet> bossBullets;
    std::vector<HealthPowerUp> healthPowerUps;

    // Timers and random
    sf::Clock bossAttackClock, bossSpreadAttackClock, bossMinionSpawnClock;
    std::mt19937 rng{ std::random_device{}() };
    sf::Clock enemySpawnClock, shootClock, shakeClock, levelTransitionClock, gameTimer, healthPowerUpClock;

    // Camera shake
    bool isShaking;
    float shakeDuration, shakeIntensity;

    // Level up/transition
    bool level1CompleteSoundPlayed, level2CompleteSoundPlayed;
    bool inLevelTransition;
    int nextLevel;

    // Boss music
    bool bossMusicStarted;

    // Level 2 difficulty
    float level2EnemySpeed, level2SpawnRate;
    int level2DoubleSpawnCounter, level2MaxEnemies;

    // Stats
    float timeSurvived;
    int totalKills;

    // Health PowerUp
    float healthPowerUpTimer;
    float healthPowerUpNextSpawn;
    
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    
  
    // --- Resource Loading and UI Setup ---
    void loadResources() {
        // Font
        font.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/main_font.ttf");
        if (!backgroundTexture.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/background.png")) {
            // Error handling if needed
        }
        backgroundSprite.setTexture(backgroundTexture);
        // Sounds
        bgBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/background_music.wav");
        shootBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/shoot.wav");
        explosionBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/explosion.wav");
        winBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/win.wav");
        loseBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/lost.wav");
        levelUpBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/levelup.wav");
        levelCompleteBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/levelcom.wav");
        bossBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/boss.wav");
        // Health powerup sound (optional, fallback to shoot if not found)
        if (!healthPowerUpBuffer.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/explosion.wav")) {
            healthPowerUpSound.setBuffer(shootBuffer);
        }
        else {
            healthPowerUpSound.setBuffer(healthPowerUpBuffer);
        }

        backgroundSound.setBuffer(bgBuffer);
        shootSound.setBuffer(shootBuffer);
        explosionSound.setBuffer(explosionBuffer);
        winSound.setBuffer(winBuffer);
        loseSound.setBuffer(loseBuffer);
        levelUpSound.setBuffer(levelUpBuffer);
        levelCompleteSound.setBuffer(levelCompleteBuffer);
        bossSound.setBuffer(bossBuffer);

        backgroundSound.setLoop(true);
        backgroundSound.setVolume(40);
        if (backgroundSound.getStatus() != sf::Sound::Playing)
            backgroundSound.play();

        bossSound.setLoop(true);
        bossSound.setVolume(60);

        // Textures
        playerTex.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/sprite_ship_3.png");
        enemyTex.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/big_boss1.png");
        bossTex.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/final_boss_animation/sprite_0.png");

        bulletFrames.resize(5);
        for (int i = 0; i < 5; ++i) {
            bulletFrames[i].loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/blaster_player/sprite_" + std::to_string(i) + ".png");
        }
        level2EnemyFrames.resize(5);
        for (int i = 0; i < 5; ++i) {
            level2EnemyFrames[i].loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/invader_animation_2/sprite_" + std::to_string(i) + ".png");
        }
        // Health powerup texture
        healthPowerUpTex.loadFromFile("D:/OOP Project/Space Invaders/Space Invaders/assets/health.png");
        player.setTexture(playerTex);
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        backgroundSprite.setScale(
            static_cast<float>(windowSize.x) / textureSize.x,
            static_cast<float>(windowSize.y) / textureSize.y
        );
    }

    void setupUI() {
        startText = sf::Text("Press SPACE to Start", font, fontSizeTitle);
        startText.setPosition(220.f, 300.f);

        winText = sf::Text("YOU WON!\nPress R to Restart", font, fontSizeTitle);
        winText.setFillColor(sf::Color::Green);
        winText.setPosition(220.f, 250.f);

        loseText = sf::Text("YOU LOSE!\nPress R to Retry", font, fontSizeTitle);
        loseText.setFillColor(sf::Color::Red);
        loseText.setPosition(220.f, 250.f);

        scoreText = sf::Text("Score: 0", font, fontSizeMain);
        scoreText.setPosition(10.f, 10.f);

        levelText = sf::Text("Level 1", font, fontSizeMain);
        levelText.setPosition(10.f, 35.f);

        healthLabelText = sf::Text("Health:", font, fontSizeMain);
        healthLabelText.setPosition(580.f - healthLabelText.getLocalBounds().width - 10.f, 10.f + (20.f - fontSizeMain) / 2.f);

        healthValueText = sf::Text(std::to_string(GameConfig::PLAYER_MAX_HEALTH), font, fontSizeSmall);
        healthValueText.setFillColor(sf::Color::Black);
        healthValueText.setPosition(580.f + GameConfig::HEALTH_BAR_WIDTH / 2.f - 10.f, 10.f + GameConfig::HEALTH_BAR_HEIGHT / 2.f - fontSizeSmall / 2.f);

        healthBarBack = sf::RectangleShape(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH, GameConfig::HEALTH_BAR_HEIGHT));
        healthBarBack.setFillColor(sf::Color(50, 50, 50));
        healthBarBack.setPosition(580.f, 10.f);

        healthBarFront = sf::RectangleShape(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH, GameConfig::HEALTH_BAR_HEIGHT));
        healthBarFront.setFillColor(sf::Color::Red);
        healthBarFront.setPosition(580.f, 10.f);

        bossHealthLabelText = sf::Text("Boss:", font, fontSizeMain);
        bossHealthLabelText.setPosition(580.f - bossHealthLabelText.getLocalBounds().width - 10.f, 40.f + (20.f - fontSizeMain) / 2.f);

        bossHealthValueText = sf::Text(std::to_string(GameConfig::BOSS_MAX_HEALTH), font, fontSizeSmall);
        bossHealthValueText.setFillColor(sf::Color::Black);
        bossHealthValueText.setPosition(580.f + GameConfig::HEALTH_BAR_WIDTH / 2.f - 10.f, 40.f + GameConfig::HEALTH_BAR_HEIGHT / 2.f - fontSizeSmall / 2.f);

        bossHealthBarBack = sf::RectangleShape(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH, GameConfig::HEALTH_BAR_HEIGHT));
        bossHealthBarBack.setFillColor(sf::Color(50, 50, 50));
        bossHealthBarBack.setPosition(580.f, 40.f);

        bossHealthBarFront = sf::RectangleShape(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH, GameConfig::HEALTH_BAR_HEIGHT));
        bossHealthBarFront.setFillColor(sf::Color::Blue);
        bossHealthBarFront.setPosition(580.f, 40.f);

        highScore = getHighScore(scoreFile);
        highScoreText = sf::Text("High Score: " + std::to_string(highScore), font, fontSizeMain);
        highScoreText.setFillColor(sf::Color::Yellow);
        highScoreText.setPosition(10.f, 60.f);

        transitionText = sf::Text("", font, fontSizeTitle);
        transitionText.setFillColor(sf::Color::Yellow);
        transitionText.setStyle(sf::Text::Bold);
        transitionText.setPosition(300.f, 250.f);
    }

    // --- Main Game Loop Methods ---
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed)
                window.close();
    }

    void update() {
        window.setView(window.getDefaultView());
        handleCameraShake();

        switch (gameState) {
        case START: handleStartScreen(); break;
        case LEVEL_TRANSITION: handleLevelTransition(); break;
        case YOU_WON:
        case YOU_LOSE: handleGameOver(); break;
        case PLAYING: handleGameplay(); break;
        default: break;
        }
    }

    void render() {
        window.clear();
        window.draw(backgroundSprite);  // draw background first
        // draw other game entities next (player, bullets, enemies, etc.)
        switch (gameState) {
        case START:
            window.draw(startText);
            window.draw(highScoreText);
            break;
        case LEVEL_TRANSITION:
            window.draw(transitionText);
            window.draw(highScoreText);
            break;
        case YOU_WON:
        case YOU_LOSE:
            renderGameOver();
            break;
        case PLAYING:
            renderGameplay();
            break;
        default: break;
        }
        window.display();
    }

    void cleanup() {
        for (auto* e : enemies) delete e;
    }

    // --- Camera Shake ---
    void handleCameraShake() {
        if (isShaking && shakeClock.getElapsedTime().asSeconds() < shakeDuration) {
            float offsetX = (rand() % 11 - 5) * shakeIntensity * 0.1f;
            float offsetY = (rand() % 11 - 5) * shakeIntensity * 0.1f;
            sf::View shakeView = window.getDefaultView();
            shakeView.move(offsetX, offsetY);
            window.setView(shakeView);
        }
        else {
            isShaking = false;
        }
    }

    // --- State Handlers ---
    void handleStartScreen() {
        gameTimer.restart();
        totalKills = 0;
        highScore = getHighScore(scoreFile);
        highScoreText.setString("High Score: " + std::to_string(highScore));
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) gameState = PLAYING;
    }

    void handleLevelTransition() {
        if (levelTransitionClock.getElapsedTime().asSeconds() > 1.0f) {
            gameState = PLAYING;
            inLevelTransition = false;
        }
    }

    void handleGameOver() {
        static bool soundPlayed = false;
        static bool scoreSaved = false;
        if (!soundPlayed) {
            if (backgroundSound.getStatus() == sf::Sound::Playing)
                backgroundSound.stop();
            if (bossSound.getStatus() == sf::Sound::Playing)
                bossSound.stop();
            if (gameState == YOU_WON) {
                if (winSound.getStatus() != sf::Sound::Playing)
                    winSound.play();
            }
            else {
                if (loseSound.getStatus() != sf::Sound::Playing)
                    loseSound.play();
            }
            soundPlayed = true;
        }
        if (!scoreSaved) {
            appendScoreToFile(scoreFile, score);
            highScore = getHighScore(scoreFile);
            highScoreText.setString("High Score: " + std::to_string(highScore));
            scoreSaved = true;
            timeSurvived = gameTimer.getElapsedTime().asSeconds();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
            resetGame();
            soundPlayed = false;
            scoreSaved = false;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
            window.close();
        }
    }

    void handleGameplay() {
        handlePlayerInput();
        handleShooting();
        handleEnemySpawning();
        handleLevelProgression();
        handleBossSpawning();
        updateEntities();
        handleBossBehavior();
        handleCollisions();
        handleHealthPowerUpSpawning();
        updateHealthPowerUps();
    }

    // --- Gameplay Logic ---
    void handlePlayerInput() {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && player.getPosition().x > 0)
            player.move(-0.5f, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && player.getPosition().x + player.getGlobalBounds().width < window.getSize().x)
            player.move(0.5f, 0.f);
    }

    void handleShooting() {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shootClock.getElapsedTime().asMilliseconds() > 200) {
            sf::FloatRect playerBounds = player.getGlobalBounds();
            sf::Vector2f bulletPos(
                playerBounds.left + playerBounds.width / 2.f,
                playerBounds.top
            );
            sf::Sprite tempBulletSprite;
            tempBulletSprite.setTexture(bulletFrames[0]);
            sf::FloatRect bulletBounds = tempBulletSprite.getGlobalBounds();
            bulletPos.x -= bulletBounds.width / 2.f;
            bullets.emplace_back(bulletFrames, bulletPos);
            if (shootSound.getStatus() != sf::Sound::Playing)
                shootSound.play();
            shootClock.restart();
        }
    }

    void handleEnemySpawning() {
        float spawnRate = (level == 1) ? GameConfig::ENEMY_SPAWN_RATE_LEVEL1 :
            (level == 2) ? level2SpawnRate : GameConfig::ENEMY_SPAWN_RATE_DEFAULT;
        if (enemySpawnClock.getElapsedTime().asSeconds() > spawnRate && level < 3) {
            float x = static_cast<float>(rand() % 750);
            if (level == 2) {
                int currentLevel2Enemies = 0;
                for (auto* e : enemies) {
                    if (dynamic_cast<AnimatedEnemy*>(e)) currentLevel2Enemies++;
                }
                if (currentLevel2Enemies < level2MaxEnemies) {
                    enemies.push_back(new AnimatedEnemy(level2EnemyFrames, sf::Vector2f(x, -50.f), level2EnemySpeed));
                    level2DoubleSpawnCounter++;
                    if (level2DoubleSpawnCounter % GameConfig::LEVEL2_DOUBLE_SPAWN_FREQ == 0 && currentLevel2Enemies + 1 < level2MaxEnemies) {
                        float x2 = static_cast<float>(rand() % 750);
                        enemies.push_back(new AnimatedEnemy(level2EnemyFrames, sf::Vector2f(x2, -50.f), level2EnemySpeed + GameConfig::LEVEL2_DOUBLE_SPAWN_SPEED_BONUS));
                    }
                }
            }
            else {
                enemies.push_back(new Enemy(enemyTex, sf::Vector2f(x, -50.f)));
            }
            enemySpawnClock.restart();
        }
    }

    void handleLevelProgression() {
        if (level == 1 && score >= GameConfig::LEVEL2_SCORE_THRESHOLD && !inLevelTransition) {
            level = 2;
            levelText.setString("Level 2");
            if (!level1CompleteSoundPlayed) {
                if (levelUpSound.getStatus() != sf::Sound::Playing)
                    levelUpSound.play();
                level1CompleteSoundPlayed = true;
            }
            transitionText.setString("Level 2");
            gameState = LEVEL_TRANSITION;
            levelTransitionClock.restart();
            inLevelTransition = true;
        }
        else if (level == 2 && score >= GameConfig::LEVEL3_SCORE_THRESHOLD && !inLevelTransition) {
            level = 3;
            levelText.setString("Final Boss!");
            if (!level2CompleteSoundPlayed) {
                if (levelCompleteSound.getStatus() != sf::Sound::Playing)
                    levelCompleteSound.play();
                level2CompleteSoundPlayed = true;
            }
            transitionText.setString("Final Level");
            gameState = LEVEL_TRANSITION;
            levelTransitionClock.restart();
            inLevelTransition = true;
            for (auto* e : enemies) delete e;
            enemies.clear();
        }
    }

    void handleBossSpawning() {
        if (level == 3 && !bossSpawned) {
            boss = Boss();
            boss.sprite.setPosition(300.f, -100.f);
            boss.health = GameConfig::BOSS_MAX_HEALTH;
            bossHealth = GameConfig::BOSS_MAX_HEALTH;
            bossHealthBarFront.setSize(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH, GameConfig::HEALTH_BAR_HEIGHT));
            bossHealthValueText.setString(std::to_string(GameConfig::BOSS_MAX_HEALTH));
            bossSpawned = true;
            if (backgroundSound.getStatus() == sf::Sound::Playing)
                backgroundSound.stop();
            if (bossSound.getStatus() != sf::Sound::Playing)
                bossSound.play();
            bossMusicStarted = true;
            bossAttackClock.restart();
            bossSpreadAttackClock.restart();
            bossMinionSpawnClock.restart();
        }
        if ((gameState == YOU_WON || gameState == YOU_LOSE) && bossMusicStarted) {
            if (bossSound.getStatus() == sf::Sound::Playing)
                bossSound.stop();
            bossMusicStarted = false;
        }
    }

    void updateEntities() {
        for (auto& bullet : bullets) bullet.update();
        for (auto* enemy : enemies) enemy->update();
        for (auto& explosion : explosions) explosion.update();
        for (auto& b : bossBullets) b.update();
    }

    void handleBossBehavior() {
        if (bossSpawned && !bossDefeated) {
            boss.update();
            if (boss.sprite.getPosition().y < GameConfig::BOSS_DESCEND_Y)
                boss.sprite.move(0.f, GameConfig::BOSS_DESCEND_SPEED);
            boss.updatePhase();

            float bossBulletSpeed = GameConfig::BOSS_BULLET_SPEED;
            float bossAttackInterval = GameConfig::BOSS_ATTACK_INTERVAL;
            if (boss.phase == Boss::PHASE2) {
                bossBulletSpeed = GameConfig::BOSS_BULLET_SPEED_PHASE2;
                bossAttackInterval = GameConfig::BOSS_ATTACK_INTERVAL_PHASE2;
            }
            else if (boss.phase == Boss::PHASE3) {
                bossBulletSpeed = GameConfig::BOSS_BULLET_SPEED_PHASE3;
                bossAttackInterval = GameConfig::BOSS_ATTACK_INTERVAL_PHASE3;
            }

            if (bossAttackClock.getElapsedTime().asSeconds() > bossAttackInterval) {
                sf::Vector2f bossPos = boss.sprite.getPosition() + sf::Vector2f(40.f, 60.f);
                bossBullets.emplace_back(bulletFrames, bossPos, sf::Vector2f(0.f, bossBulletSpeed));
                bossAttackClock.restart();
            }

            if (boss.phase >= Boss::PHASE2 && bossSpreadAttackClock.getElapsedTime().asSeconds() > GameConfig::BOSS_SPREAD_BULLET_INTERVAL) {
                sf::Vector2f bossPos = boss.sprite.getPosition() + sf::Vector2f(40.f, 60.f);
                int n = GameConfig::BOSS_SPREAD_BULLET_COUNT;
                float spread = GameConfig::BOSS_SPREAD_BULLET_ANGLE;
                float angleStart = 90.f - spread / 2.f;
                float angleStep = spread / (n - 1);
                for (int i = 0; i < n; ++i) {
                    float angle = angleStart + i * angleStep;
                    float rad = angle * 3.14159265f / 180.f;
                    sf::Vector2f vel(std::cos(rad) * bossBulletSpeed, std::sin(rad) * bossBulletSpeed);
                    bossBullets.emplace_back(bulletFrames, bossPos, vel);
                }
                bossSpreadAttackClock.restart();
            }

            if (boss.phase == Boss::PHASE3 && bossMinionSpawnClock.getElapsedTime().asSeconds() > GameConfig::BOSS_MINION_SPAWN_INTERVAL) {
                std::uniform_int_distribution<int> minionCountDist(1, 2);
                int minionCount = minionCountDist(rng);
                for (int i = 0; i < minionCount; ++i) {
                    float minionX = boss.sprite.getPosition().x + 20.f + (rand() % 60) - 30.f;
                    minionX = std::max(0.f, std::min(minionX, (float)(GameConfig::WINDOW_WIDTH - 40)));
                    enemies.push_back(new AnimatedEnemy(level2EnemyFrames, sf::Vector2f(minionX, boss.sprite.getPosition().y + 80.f), level2EnemySpeed + 0.1f));
                }
                bossMinionSpawnClock.restart();
            }

            for (size_t i = 0; i < bullets.size(); ++i) {
                if (bullets[i].sprite.getGlobalBounds().intersects(boss.sprite.getGlobalBounds())) {
                    bullets.erase(bullets.begin() + i);
                    boss.health -= GameConfig::BOSS_HIT_DAMAGE;
                    bossHealth -= GameConfig::BOSS_HIT_DAMAGE;
                    if (explosionSound.getStatus() != sf::Sound::Playing)
                        explosionSound.play();
                    isShaking = true;
                    shakeClock.restart();
                    float bossBarWidth = std::max(0.f, GameConfig::HEALTH_BAR_WIDTH * (float)boss.health / GameConfig::BOSS_MAX_HEALTH);
                    bossHealthBarFront.setSize(sf::Vector2f(bossBarWidth, GameConfig::HEALTH_BAR_HEIGHT));
                    bossHealthValueText.setString(std::to_string(std::max(0, boss.health)));
                    break;
                }
            }
            if (boss.health <= 0) {
                if (boss.sprite.getPosition().y > GameConfig::WINDOW_HEIGHT) {
                    explosions.emplace_back(boss.sprite.getPosition());
                }
                bossDefeated = true;
                gameState = YOU_WON;
            }
        }
    }

    void handleCollisions() {
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [](Bullet& b) { return b.sprite.getPosition().y < GameConfig::BULLET_OFFSCREEN_Y; }), bullets.end());
        bossBullets.erase(std::remove_if(bossBullets.begin(), bossBullets.end(),
            [](Bullet& b) {
                sf::Vector2f pos = b.sprite.getPosition();
                return pos.y > GameConfig::WINDOW_HEIGHT || pos.x < 0 || pos.x > GameConfig::WINDOW_WIDTH;
            }), bossBullets.end());
        explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
            [](Explosion& e) { return e.finished; }), explosions.end());

        // --- Health PowerUp collision with bullets ---
        for (size_t i = 0; i < bullets.size(); ++i) {
            bool bulletErased = false;
            // Check collision with health powerups
            for (size_t h = 0; h < healthPowerUps.size(); ++h) {
                if (healthPowerUps[h].active && bullets[i].sprite.getGlobalBounds().intersects(healthPowerUps[h].sprite.getGlobalBounds())) {
                    // Play powerup sound
                    if (healthPowerUpSound.getStatus() != sf::Sound::Playing)
                        healthPowerUpSound.play();
                    // Replenish health
                    health += GameConfig::HEALTH_POWERUP_AMOUNT;
                    if (health > GameConfig::PLAYER_MAX_HEALTH)
                        health = GameConfig::PLAYER_MAX_HEALTH;
                    healthBarFront.setSize(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH * (float)health / GameConfig::PLAYER_MAX_HEALTH, GameConfig::HEALTH_BAR_HEIGHT));
                    healthValueText.setString(std::to_string(health));
                    // Remove bullet and powerup
                    bullets.erase(bullets.begin() + i);
                    healthPowerUps[h].active = false;
                    bulletErased = true;
                    break;
                }
            }
            if (bulletErased) {
                i--;
                continue;
            }
            // --- Enemy collision ---
            for (size_t j = 0; j < enemies.size(); ++j) {
                if (bullets[i].sprite.getGlobalBounds().intersects(enemies[j]->sprite.getGlobalBounds())) {
                    explosions.emplace_back(enemies[j]->sprite.getPosition());
                    delete enemies[j];
                    enemies.erase(enemies.begin() + j);
                    bullets.erase(bullets.begin() + i);
                    if (explosionSound.getStatus() != sf::Sound::Playing)
                        explosionSound.play();
                    isShaking = true;
                    shakeDuration = GameConfig::SHAKE_DURATION;
                    shakeClock.restart();
                    score += 10;
                    scoreText.setString("Score: " + std::to_string(score));
                    healthValueText.setString(std::to_string(health));
                    if (score > highScore) {
                        highScore = score;
                        highScoreText.setString("High Score: " + std::to_string(highScore));
                    }
                    bulletErased = true;
                    i--;
                    totalKills++;
                    break;
                }
            }
            if (bulletErased) break;
        }
        for (auto it = bossBullets.begin(); it != bossBullets.end();) {
            if (it->sprite.getGlobalBounds().intersects(player.getGlobalBounds())) {
                it = bossBullets.erase(it);
                health -= GameConfig::BOSS_BULLET_DAMAGE;
                healthBarFront.setSize(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH * (float)health / GameConfig::PLAYER_MAX_HEALTH, GameConfig::HEALTH_BAR_HEIGHT));
                healthValueText.setString(std::to_string(std::max(0, health)));
                if (health <= 0) gameState = YOU_LOSE;
            }
            else {
                ++it;
            }
        }
        for (auto it = enemies.begin(); it != enemies.end();) {
            if ((*it)->sprite.getPosition().y > GameConfig::WINDOW_HEIGHT) {
                explosions.emplace_back((*it)->sprite.getPosition());
                if (explosionSound.getStatus() != sf::Sound::Playing)
                    explosionSound.play();
                isShaking = true;
                shakeDuration = GameConfig::SHAKE_DURATION;
                shakeClock.restart();
                delete* it;
                it = enemies.erase(it);
                if (level == 1) {
                    health -= GameConfig::ENEMY_DAMAGE_LEVEL1;
                }
                else if (level == 2) {
                    health -= GameConfig::ENEMY_DAMAGE_LEVEL2;
                }
                else {
                    health -= GameConfig::ENEMY_DAMAGE_DEFAULT;
                }
                healthBarFront.setSize(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH * (float)health / GameConfig::PLAYER_MAX_HEALTH, GameConfig::HEALTH_BAR_HEIGHT));
                healthValueText.setString(std::to_string(std::max(0, health)));
                if (health <= 0) gameState = YOU_LOSE;
            }
            else ++it;
        }
        // Remove inactive health powerups
        healthPowerUps.erase(
            std::remove_if(healthPowerUps.begin(), healthPowerUps.end(),
                [](const HealthPowerUp& h) { return !h.active; }),
            healthPowerUps.end()
        );
    }

    // --- Health PowerUp Spawning and Update ---
    void handleHealthPowerUpSpawning() {
        // Only spawn health powerup if health is below 50
        if (health < 50) {
            float elapsed = healthPowerUpClock.getElapsedTime().asSeconds();
            if (elapsed > healthPowerUpNextSpawn) {
                float x = static_cast<float>(rand() % (GameConfig::WINDOW_WIDTH - 40) + 20);
                healthPowerUps.emplace_back(healthPowerUpTex, sf::Vector2f(x, -40.f), GameConfig::HEALTH_POWERUP_SPEED);
                healthPowerUpClock.restart();
                scheduleNextHealthPowerUp();
            }
        }
    }
    void updateHealthPowerUps() {
        for (auto& h : healthPowerUps) {
            if (h.active)
                h.update();
        }
    }
    void scheduleNextHealthPowerUp() {
        std::uniform_real_distribution<float> dist(GameConfig::HEALTH_POWERUP_SPAWN_MIN, GameConfig::HEALTH_POWERUP_SPAWN_MAX);
        healthPowerUpNextSpawn = dist(rng);
        healthPowerUpClock.restart();
    }

    // --- Rendering ---
    void renderGameplay() {
        window.draw(player);
        window.draw(healthBarBack);
        window.draw(healthBarFront);
        window.draw(healthLabelText);
        window.draw(healthValueText);
        if (bossSpawned && !bossDefeated) {
            window.draw(bossHealthBarBack);
            window.draw(bossHealthBarFront);
            window.draw(bossHealthLabelText);
            window.draw(bossHealthValueText);
        }
        for (auto& bullet : bullets) window.draw(bullet.sprite);
        for (auto* enemy : enemies) window.draw(enemy->sprite);
        for (auto& explosion : explosions) window.draw(explosion.sprite);
        // Draw health powerups only if health is below 50
        if (health < 50) {
            for (auto& h : healthPowerUps)
                if (h.active)
                    window.draw(h.sprite);
        }
        if (bossSpawned && !bossDefeated)
            window.draw(boss.sprite);
        for (auto& b : bossBullets)
            window.draw(b.sprite);
        window.draw(scoreText);
        window.draw(levelText);
        window.draw(highScoreText);
    }

    void renderGameOver() {
        if (gameState == YOU_WON)
            window.draw(winText);
        else
            window.draw(loseText);
        window.draw(scoreText);
        window.draw(highScoreText);

        char timeBuffer[64];
        int minutes = static_cast<int>(timeSurvived) / 60;
        int seconds = static_cast<int>(timeSurvived) % 60;
        snprintf(timeBuffer, sizeof(timeBuffer), "Time Survived: %02d:%02d", minutes, seconds);

        sf::Text timeText(timeBuffer, font, fontSizeMain);
        timeText.setFillColor(sf::Color::Cyan);
        timeText.setPosition(220.f, 320.f);

        sf::Text killsText("Total Kills: " + std::to_string(totalKills), font, fontSizeMain);
        killsText.setFillColor(sf::Color::Magenta);
        killsText.setPosition(220.f, 350.f);

        sf::Text quitText("Press Q to Quit", font, fontSizeMain);
        quitText.setFillColor(sf::Color(200, 200, 200));
        quitText.setPosition(220.f, 380.f);

        window.draw(timeText);
        window.draw(killsText);
        window.draw(quitText);
    }

    // --- Reset ---
    void resetGame() {
        health = GameConfig::PLAYER_MAX_HEALTH;
        score = 0;
        level = 1;
        for (auto* e : enemies) delete e;
        enemies.clear();
        bullets.clear();
        explosions.clear();
        bossSpawned = false;
        bossDefeated = false;
        bossHealth = GameConfig::BOSS_MAX_HEALTH;
        boss.health = GameConfig::BOSS_MAX_HEALTH;
        scoreText.setString("Score: 0");
        levelText.setString("Level 1");
        healthBarFront.setSize(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH, GameConfig::HEALTH_BAR_HEIGHT));
        healthValueText.setString(std::to_string(GameConfig::PLAYER_MAX_HEALTH));
        bossHealthBarFront.setSize(sf::Vector2f(GameConfig::HEALTH_BAR_WIDTH, GameConfig::HEALTH_BAR_HEIGHT));
        bossHealthValueText.setString(std::to_string(GameConfig::BOSS_MAX_HEALTH));
        level1CompleteSoundPlayed = false;
        level2CompleteSoundPlayed = false;
        bossMusicStarted = false;
        gameTimer.restart();
        totalKills = 0;
        highScore = getHighScore(scoreFile);
        highScoreText.setString("High Score: " + std::to_string(highScore));
        if (backgroundSound.getStatus() != sf::Sound::Playing)
            backgroundSound.play();
        // Reset health powerups
        healthPowerUps.clear();
        scheduleNextHealthPowerUp();
        gameState = PLAYING;
    }
};

// --- Main Entry Point ---
int main() {
    SpaceInvadersGame game;
    game.run();
    return 0;
}