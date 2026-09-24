#pragma once

#include <cstdint>
#include <deque>
#include <random>

namespace ll {

// Источник случайности. Через интерфейс — чтобы тесты и запуски с --seed были детерминированными.
class IRandom {
public:
    virtual ~IRandom() = default;
    virtual int range(int lo, int hi) = 0;  // целое из [lo, hi]
    virtual bool chance(double p) = 0;      // true с вероятностью p
};

class Random final : public IRandom {
public:
    explicit Random(std::uint32_t seed);
    int range(int lo, int hi) override;
    bool chance(double p) override;
    void reseed(std::uint32_t seed) { engine_.seed(seed); }

private:
    std::mt19937 engine_;
};

// Для тестов: заранее заданные результаты; когда очередь пуста —
// range возвращает lo, chance возвращает false.
class FixedRandom final : public IRandom {
public:
    void pushRange(int value) { ranges_.push_back(value); }
    void pushChance(bool value) { chances_.push_back(value); }
    int range(int lo, int hi) override;
    bool chance(double p) override;

private:
    std::deque<int> ranges_;
    std::deque<bool> chances_;
};

}  // namespace ll
