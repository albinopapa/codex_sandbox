#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "camera.hpp"
#include "display.hpp"
#include "font.hpp"
#include "frustum.hpp"
#include "geometry.hpp"
#include "renderer.hpp"

struct DemoConfig {
    static constexpr bool enableTexturing = true;
    static constexpr bool enableDepthTest = true;
    static constexpr bool enableBackfaceCulling = true;
};

class Timer {
public:
    explicit Timer(std::chrono::milliseconds frameDuration)
        : frameDuration_(frameDuration), nextFrameTime_(Clock::now() + frameDuration_) {}

    void waitForNextFrame() {
        std::this_thread::sleep_until(nextFrameTime_);
        nextFrameTime_ += frameDuration_;

        const auto now = Clock::now();
        if (now > nextFrameTime_) {
            nextFrameTime_ = now + frameDuration_;
        }
    }

private:
    using Clock = std::chrono::steady_clock;

    std::chrono::milliseconds frameDuration_;
    Clock::time_point nextFrameTime_;
};

enum class GameState {
    Title,
    Playing,
    Victory,
    GameOver,
};

struct Projectile {
    la::Vec3<float> pixelPosition{0.0f, 0.0f, 0.0f};
    la::Vec3<float> pixelVelocity{0.0f, 0.0f, 0.0f};
    float radius = 0.08f;
    bool fromPlayer = true;
    bool active = false;
};

struct Enemy {
    la::Vec3<float> pixelPosition{0.0f, 0.0f, 0.0f};
    la::Vec3<float> targetPixelPosition{0.0f, 0.0f, 0.0f};
    std::array<la::Vec3<float>, 4> flyInCurve{};
    float flyInT = 0.0f;
    std::size_t formationRow = 0;
    std::size_t formationCol = 0;
    bool inFormation = false;
    gfx::Colorf color{1.0f, 1.0f, 1.0f, 1.0f};
    float size = 0.25f;
    float radius = 0.2f;
    bool alive = true;
};

class World {
public:
    static constexpr float baseCanvasSize = 512.0f;
    static constexpr float playMinX = -2.5f;
    static constexpr float playMaxX = 2.5f;
    static constexpr float playMinY = -2.1f;
    static constexpr float playMaxY = 2.1f;
    static constexpr float playerY = -1.85f;
    static constexpr float enemyBottomY = -1.35f;
    void setCanvasSize(float width, float height) {
        canvasWidth_ = std::max(1.0f, width);
        canvasHeight_ = std::max(1.0f, height);
        playerPixelY_ = canvasHeight_ * 0.9f;
        playerPixelX_ = std::clamp(playerPixelX_, 0.0f, canvasWidth_);
    }

    void reset() {
        score_ = 0;
        lives_ = 3;
        playerPixelX_ = canvasWidth_ * 0.5f;
        playerPixelY_ = canvasHeight_ * 0.9f;
        enemyDirection_ = 1.0f;
        shotCooldown_ = 0.0f;
        enemyShotCooldown_ = 0.75f;
        enemyColumnCursor_ = 0;
        clearProjectiles();
        spawnEnemies();
    }

    void update(float dt, const gfx::Keyboard& keyboard) {
        if (shotCooldown_ > 0.0f) {
            shotCooldown_ -= dt;
        }
        if (enemyShotCooldown_ > 0.0f) {
            enemyShotCooldown_ -= dt;
        }

        updatePlayer(dt, keyboard);
        updateEnemies(dt);
        updateProjectiles(dt);
        resolveCollisions();
        fireEnemyProjectile();
    }

    bool hasWon() const {
        for (const auto& enemy : enemies_) {
            if (enemy.alive) {
                return false;
            }
        }
        return true;
    }

    bool hasLost() const {
        return lives_ <= 0 || enemyReachedPlayerRow_;
    }

    int score() const { return score_; }
    int lives() const { return lives_; }
    la::Vec3<float> playerWorldPosition() const { return worldFromPixel(playerPixelX_, playerPixelY_); }

    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }

    la::Vec3<float> worldFromPixel(float x, float y, float z = -4.0f) const {
        const float nx = x / canvasWidth_;
        const float ny = y / canvasHeight_;
        const float wx = playMinX + nx * (playMaxX - playMinX);
        const float wy = playMaxY - ny * (playMaxY - playMinY);
        return {wx, wy, z};
    }

    float pixelDistanceFromWorldX(float worldDistance) const {
        return worldDistance * canvasWidth_ / (playMaxX - playMinX);
    }

    float pixelDistanceFromWorldY(float worldDistance) const {
        return worldDistance * canvasHeight_ / (playMaxY - playMinY);
    }

private:
    static la::Vec3<float> curve(const std::array<la::Vec3<float>, 4>& points, float t) {
        const float clampedT = std::clamp(t, 0.0f, 1.0f);
        const float omt = 1.0f - clampedT;
        return points[0] * (omt * omt * omt)
            + points[1] * (3.0f * omt * omt * clampedT)
            + points[2] * (3.0f * omt * clampedT * clampedT)
            + points[3] * (clampedT * clampedT * clampedT);
    }

    void clearProjectiles() {
        projectiles_.clear();
        projectiles_.reserve(64);
    }

    bool isPressed(const gfx::Keyboard& keyboard, std::uint32_t a, std::uint32_t b, std::uint32_t c = 0) const {
        return keyboard.isKeyDown(a) || keyboard.isKeyDown(b) || (c != 0 && keyboard.isKeyDown(c));
    }

    void updatePlayer(float dt, const gfx::Keyboard& keyboard) {
        constexpr std::uint32_t keyLeft = 0x25;
        constexpr std::uint32_t keyRight = 0x27;
        constexpr std::uint32_t keySpace = 0x20;

        const bool moveLeft = isPressed(keyboard, keyLeft, 'A', 'a');
        const bool moveRight = isPressed(keyboard, keyRight, 'D', 'd');

        float move = 0.0f;
        if (moveLeft) {
            move -= 1.0f;
        }
        if (moveRight) {
            move += 1.0f;
        }

        constexpr float playerSpeedPx = 280.0f;
        playerPixelX_ += move * playerSpeedPx * dt;
        playerPixelX_ = std::clamp(playerPixelX_, 0.0f, canvasWidth_);

        if (keyboard.isKeyDown(keySpace) && shotCooldown_ <= 0.0f) {
            spawnProjectile({playerPixelX_, playerPixelY_ - 14.0f, -4.0f}, {0.0f, -440.0f, 0.0f}, true);
            shotCooldown_ = 0.3f;
        }
    }

    void updateEnemies(float dt) {
        constexpr float enemyFlyInSpeed = 0.55f;
        bool anyFlying = false;

        for (auto& enemy : enemies_) {
            if (!enemy.alive || enemy.inFormation) {
                continue;
            }
            anyFlying = true;
            enemy.flyInT += dt * enemyFlyInSpeed;
            enemy.pixelPosition = curve(enemy.flyInCurve, enemy.flyInT);
            if (enemy.flyInT >= 1.0f) {
                enemy.inFormation = true;
                enemy.pixelPosition = enemy.targetPixelPosition;
            }
        }

        if (anyFlying) {
            return;
        }

        constexpr float enemySpeedPx = 78.0f;
        constexpr float enemyStepDownPx = 18.0f;

        float minX = 999.0f;
        float maxX = -999.0f;
        bool haveAlive = false;

        for (auto& enemy : enemies_) {
            if (!enemy.alive) {
                continue;
            }
            haveAlive = true;
            enemy.pixelPosition.x += enemyDirection_ * enemySpeedPx * dt;
            enemy.targetPixelPosition.x += enemyDirection_ * enemySpeedPx * dt;

            const auto enemyWorldPos = worldFromPixel(enemy.pixelPosition.x, enemy.pixelPosition.y);
            minX = std::min(minX, enemyWorldPos.x - enemy.size * 0.5f);
            maxX = std::max(maxX, enemyWorldPos.x + enemy.size * 0.5f);
        }

        if (haveAlive && (minX < playMinX || maxX > playMaxX)) {
            enemyDirection_ *= -1.0f;
            for (auto& enemy : enemies_) {
                if (enemy.alive) {
                    enemy.pixelPosition.y += enemyStepDownPx;
                    enemy.targetPixelPosition.y += enemyStepDownPx;

                    const auto enemyWorldPos = worldFromPixel(enemy.pixelPosition.x, enemy.pixelPosition.y);
                    if (enemyWorldPos.y <= enemyBottomY) {
                        enemyReachedPlayerRow_ = true;
                    }
                }
            }
        }
    }

    void updateProjectiles(float dt) {
        for (auto& projectile : projectiles_) {
            if (!projectile.active) {
                continue;
            }
            projectile.pixelPosition += projectile.pixelVelocity * dt;
            if (projectile.pixelPosition.y < -20.0f || projectile.pixelPosition.y > canvasHeight_ + 20.0f) {
                projectile.active = false;
            }
        }
    }

    void resolveCollisions() {
        for (auto& projectile : projectiles_) {
            if (!projectile.active) {
                continue;
            }

            if (projectile.fromPlayer) {
                for (auto& enemy : enemies_) {
                    if (!enemy.alive) {
                        continue;
                    }
                    const la::Vec3<float> projectileWorldPos = worldFromPixel(projectile.pixelPosition.x, projectile.pixelPosition.y);
                    const la::Vec3<float> enemyWorldPos = worldFromPixel(enemy.pixelPosition.x, enemy.pixelPosition.y);
                    const la::Vec3<float> delta = projectileWorldPos - enemyWorldPos;
                    const float combined = projectile.radius + enemy.radius;
                    if (delta.dot(delta) <= combined * combined) {
                        enemy.alive = false;
                        projectile.active = false;
                        score_ += 100;
                        break;
                    }
                }
            } else {
                const la::Vec3<float> projectileWorldPos = worldFromPixel(projectile.pixelPosition.x, projectile.pixelPosition.y);
                la::Vec3<float> playerPos = worldFromPixel(playerPixelX_, playerPixelY_);
                const la::Vec3<float> delta = projectileWorldPos - playerPos;
                const float combined = projectile.radius + playerRadius_;
                if (delta.dot(delta) <= combined * combined) {
                    projectile.active = false;
                    --lives_;
                }
            }
        }
    }

    void fireEnemyProjectile() {
        if (enemyShotCooldown_ > 0.0f) {
            return;
        }

        if (enemies_.empty()) {
            return;
        }

        const std::size_t totalColumns = columns_;
        for (std::size_t attempt = 0; attempt < totalColumns; ++attempt) {
            const std::size_t column = (enemyColumnCursor_ + attempt) % totalColumns;
            const Enemy* shooter = nullptr;
            float lowestY = -9999.0f;
            for (const auto& enemy : enemies_) {
                if (!enemy.alive) {
                    continue;
                }
                if (enemy.formationCol != column) {
                    continue;
                }
                if (enemy.pixelPosition.y > lowestY) {
                    lowestY = enemy.pixelPosition.y;
                    shooter = &enemy;
                }
            }

            if (shooter != nullptr) {
                spawnProjectile(shooter->pixelPosition + la::Vec3<float>{0.0f, 16.0f, 0.0f}, {0.0f, 210.0f, 0.0f}, false);
                enemyColumnCursor_ = (column + 1) % totalColumns;
                enemyShotCooldown_ = 0.95f;
                return;
            }
        }

        enemyShotCooldown_ = 0.2f;
    }

    void spawnProjectile(const la::Vec3<float>& position, const la::Vec3<float>& velocity, bool fromPlayer) {
        for (auto& projectile : projectiles_) {
            if (!projectile.active) {
                projectile.pixelPosition = position;
                projectile.pixelVelocity = velocity;
                projectile.fromPlayer = fromPlayer;
                projectile.active = true;
                projectile.radius = fromPlayer ? 0.08f : 0.07f;
                return;
            }
        }

        if (projectiles_.size() >= 64) {
            return;
        }

        projectiles_.push_back(Projectile{position, velocity, fromPlayer ? 0.08f : 0.07f, fromPlayer, true});
    }

    void spawnEnemies() {
        enemies_.clear();
        enemyReachedPlayerRow_ = false;

        constexpr std::size_t rows = 4;
        columns_ = 8;
        const float enemySpacingXWorld = (50.0f / baseCanvasSize) * (playMaxX - playMinX);
        const float enemySpacingYWorld = (42.0f / baseCanvasSize) * (playMaxY - playMinY);
        const float enemySpacingX = pixelDistanceFromWorldX(enemySpacingXWorld);
        const float enemySpacingY = pixelDistanceFromWorldY(enemySpacingYWorld);
        const float formationTopY = 96.0f;
        const float formationStartX = (canvasWidth_ - (static_cast<float>(columns_ - 1) * enemySpacingX)) * 0.5f;

        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t col = 0; col < columns_; ++col) {
                Enemy enemy;
                enemy.formationRow = row;
                enemy.formationCol = col;
                enemy.targetPixelPosition = {
                    formationStartX + static_cast<float>(col) * enemySpacingX,
                    formationTopY + static_cast<float>(row) * enemySpacingY,
                    -4.0f};
                enemy.pixelPosition = {enemy.targetPixelPosition.x, -80.0f - static_cast<float>(row * 22), -4.0f};

                const float enterSide = (col % 2 == 0) ? -60.0f : (canvasWidth_ + 60.0f);
                enemy.flyInCurve = {
                    la::Vec3<float>{enterSide, -100.0f - static_cast<float>(row * 14), -4.0f},
                    la::Vec3<float>{canvasWidth_ * 0.5f + (enterSide < 0.0f ? 90.0f : -90.0f), 34.0f + static_cast<float>(row * 8), -4.0f},
                    la::Vec3<float>{enemy.targetPixelPosition.x + (enterSide < 0.0f ? -45.0f : 45.0f), enemy.targetPixelPosition.y - 65.0f, -4.0f},
                    enemy.targetPixelPosition};
                enemy.flyInT = static_cast<float>(row * columns_ + col) * 0.022f;
                enemy.inFormation = false;

                if (row == 0) {
                    enemy.size = 0.34f;
                    enemy.color = {0.98f, 0.45f, 0.35f, 1.0f};
                } else if (row < 3) {
                    enemy.size = 0.28f;
                    enemy.color = {0.95f, 0.8f, 0.26f, 1.0f};
                } else {
                    enemy.size = 0.22f;
                    enemy.color = {0.35f, 0.85f, 1.0f, 1.0f};
                }

                enemy.radius = enemy.size * 0.65f;
                enemy.alive = true;
                enemies_.push_back(enemy);
            }
        }
    }

private:
    std::vector<Enemy> enemies_;
    std::vector<Projectile> projectiles_;

    int score_ = 0;
    int lives_ = 3;

    float playerPixelX_ = 256.0f;
    float playerPixelY_ = 460.0f;
    float playerRadius_ = 0.18f;
    float canvasWidth_ = 512.0f;
    float canvasHeight_ = 512.0f;

    float enemyDirection_ = 1.0f;
    float shotCooldown_ = 0.0f;
    float enemyShotCooldown_ = 0.75f;

    bool enemyReachedPlayerRow_ = false;

    std::size_t columns_ = 8;
    std::size_t enemyColumnCursor_ = 0;
};

class Game {
public:
    using Pixel = gfx::Color8;

    Game()
        : windowSize_(computeWindowSize()),
          framebuffer_(windowSize_, windowSize_, Pixel{0, 0, 0, 255}),
          checker_(4, 4, Pixel{255, 255, 255, 255}),
          font_(gfx::Font::createAtlas("Consolas", 14)),
          renderer_(framebuffer_),
          display_(windowSize_, windowSize_, "Galaga MVP"),
          camera_({0.0f, 0.0f, 6.0f}),
          timer_(std::chrono::milliseconds(16)),
          cubeLarge_(gfx::makeCube(0.34f, 0.34f, 0.34f)),
          cubeMedium_(gfx::makeCube(0.28f, 0.28f, 0.28f)),
          cubeSmall_(gfx::makeCube(0.22f, 0.22f, 0.22f)),
          spherePlayer_(gfx::makeSphere(0.2f, 22, 16)),
          projectileMesh_(gfx::makeCube(0.08f, 0.12f, 0.08f)),
          playfield_(gfx::makePlane(5.8f, 4.8f, 1, 1)) {
        world_.setCanvasSize(static_cast<float>(windowSize_), static_cast<float>(windowSize_));
        for (std::size_t y = 0; y < checker_.height(); ++y) {
            for (std::size_t x = 0; x < checker_.width(); ++x) {
                if ((x + y) % 2 == 1) {
                    checker_.at(x, y) = Pixel{32, 32, 32, 255};
                }
            }
        }
    }

    int run() {
        world_.reset();
        std::size_t frame = 0;
#ifndef _WIN32
        constexpr std::size_t maxFrames = 420;
#endif

        while (display_.processEvents()) {
#ifndef _WIN32
            if (frame++ >= maxFrames) {
                break;
            }
#endif
            const auto& keyboard = display_.keyboard();
            handleStateTransition(keyboard);
            if (state_ == GameState::Playing) {
                world_.update(1.0f / 60.0f, keyboard);
                if (world_.hasWon()) {
                    state_ = GameState::Victory;
                } else if (world_.hasLost()) {
                    state_ = GameState::GameOver;
                }
            }

            render();
            display_.present(framebuffer_);
            timer_.waitForNextFrame();
        }

        dumpFrame();
        return 0;
    }

private:
    using RV = gfx::SoftwareRenderer<DemoConfig, Pixel>::Vertex;

    void handleStateTransition(const gfx::Keyboard& keyboard) {
        const bool startPressed = keyboard.isKeyDown(0x20);
        const bool restartPressed = keyboard.isKeyDown('R') || keyboard.isKeyDown('r');

        if (state_ == GameState::Title && startPressed) {
            world_.reset();
            state_ = GameState::Playing;
        } else if ((state_ == GameState::GameOver || state_ == GameState::Victory) && restartPressed) {
            world_.reset();
            state_ = GameState::Title;
        }
    }

    void drawMesh(const gfx::Mesh& mesh,
                  const la::Mat4<float>& viewProj,
                  const la::Vec3<float>& position,
                  const gfx::Colorf& tint,
                  const gfx::Surface<Pixel>* texture = nullptr) {
        const auto model = la::Mat4<float>::translation(position.x, position.y, position.z);
        const auto mvp = viewProj * model;
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const auto& a = mesh.vertices[mesh.indices[i]];
            const auto& b = mesh.vertices[mesh.indices[i + 1]];
            const auto& c = mesh.vertices[mesh.indices[i + 2]];
            renderer_.drawTriangle(
                RV{a.position, a.uv, tint},
                RV{b.position, b.uv, tint},
                RV{c.position, c.uv, tint},
                mvp,
                texture);
        }
    }

    void render() {
        renderer_.clear(Pixel{8, 12, 26, 255});

        const auto proj = la::Mat4<float>::perspective(1.0472f, 1.0f, 0.1f, 50.0f);
        const auto view = camera_.viewMatrix();
        const auto viewProj = proj * view;

        drawMesh(playfield_, viewProj, {0.0f, -0.2f, -4.0f}, {0.15f, 0.2f, 0.35f, 1.0f}, &checker_);
        drawMesh(spherePlayer_, viewProj, world_.playerWorldPosition(), {0.7f, 0.9f, 1.0f, 1.0f});

        for (const auto& enemy : world_.enemies()) {
            if (!enemy.alive) {
                continue;
            }

            const gfx::Mesh* mesh = &cubeSmall_;
            if (enemy.size > 0.31f) {
                mesh = &cubeLarge_;
            } else if (enemy.size > 0.25f) {
                mesh = &cubeMedium_;
            }
            drawMesh(*mesh, viewProj, world_.worldFromPixel(enemy.pixelPosition.x, enemy.pixelPosition.y), enemy.color);
        }

        for (const auto& projectile : world_.projectiles()) {
            if (!projectile.active) {
                continue;
            }
            const gfx::Colorf color = projectile.fromPlayer
                ? gfx::Colorf{0.85f, 1.0f, 0.88f, 1.0f}
                : gfx::Colorf{1.0f, 0.5f, 0.45f, 1.0f};
            drawMesh(projectileMesh_, viewProj, world_.worldFromPixel(projectile.pixelPosition.x, projectile.pixelPosition.y), color);
        }

        gfx::renderText({10, 10}, "SCORE: " + std::to_string(world_.score()), {1.0f, 1.0f, 0.85f, 1.0f}, font_, framebuffer_);
        gfx::renderText({10, 28}, "LIVES: " + std::to_string(world_.lives()), {0.85f, 0.95f, 1.0f, 1.0f}, font_, framebuffer_);

        if (state_ == GameState::Title) {
            gfx::renderText({120, 210}, "GALAGA MVP", {0.95f, 0.9f, 0.4f, 1.0f}, font_, framebuffer_);
            gfx::renderText({65, 240}, "PRESS SPACE TO START", {0.9f, 0.95f, 1.0f, 1.0f}, font_, framebuffer_);
        } else if (state_ == GameState::GameOver) {
            gfx::renderText({150, 224}, "GAME OVER", {1.0f, 0.45f, 0.45f, 1.0f}, font_, framebuffer_);
            gfx::renderText({68, 244}, "PRESS R TO RESTART", {0.9f, 0.95f, 1.0f, 1.0f}, font_, framebuffer_);
        } else if (state_ == GameState::Victory) {
            gfx::renderText({170, 224}, "YOU WIN", {0.45f, 1.0f, 0.65f, 1.0f}, font_, framebuffer_);
            gfx::renderText({68, 244}, "PRESS R TO RESTART", {0.9f, 0.95f, 1.0f, 1.0f}, font_, framebuffer_);
        }
    }

    void dumpFrame() {
        std::ofstream ppm("render.ppm");
        ppm << "P3\n" << framebuffer_.width() << ' ' << framebuffer_.height() << "\n255\n";
        for (std::size_t y = 0; y < framebuffer_.height(); ++y) {
            for (std::size_t x = 0; x < framebuffer_.width(); ++x) {
                const auto px = framebuffer_.at(x, y);
                ppm << static_cast<int>(px.r) << ' ' << static_cast<int>(px.g) << ' ' << static_cast<int>(px.b) << '\n';
            }
        }
    }

private:
    static std::size_t computeWindowSize() {
#ifdef _WIN32
        return static_cast<std::size_t>(std::max(320, GetSystemMetrics(SM_CYSCREEN)));
#else
        return 512;
#endif
    }

private:
    std::size_t windowSize_ = 512;

    gfx::Surface<Pixel> framebuffer_;
    gfx::Surface<Pixel> checker_;
    gfx::Font font_;

    gfx::SoftwareRenderer<DemoConfig, Pixel> renderer_;
    gfx::Display<Pixel> display_;
    gfx::Camera camera_;
    Timer timer_;

    gfx::Mesh cubeLarge_;
    gfx::Mesh cubeMedium_;
    gfx::Mesh cubeSmall_;
    gfx::Mesh spherePlayer_;
    gfx::Mesh projectileMesh_;
    gfx::Mesh playfield_;

    World world_;
    GameState state_ = GameState::Title;
};

int main() {
    Game game;
    return game.run();
}
