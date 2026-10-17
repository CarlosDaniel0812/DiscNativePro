#pragma once

// Motor de Beatgrid Dinámico / BPM Variable para DiscNativePro (C++20 / JUCE 8)
// Modela la cuadrícula elástica de tiempo para temas con tempo fluctuante
// (bateristas humanos, Disco, Funk, Rock, Salsa o transiciones de tempo).

#include <vector>
#include <cmath>
#include <algorithm>

struct BeatMarker
{
    double timeSeconds { 0.0 };  // Posición temporal exacta en segundos del golpe
    double instantBpm { 120.0 };  // BPM instantáneo en este intervalo
    int beatNumber { 1 };         // 1, 2, 3 o 4 (número de pulso en compás 4/4)
};

class DynamicBeatgrid
{
public:
    DynamicBeatgrid() = default;

    void clear()
    {
        markers.clear();
        averageBpm = 120.0;
        minBpm = 120.0;
        maxBpm = 120.0;
        hasVariableTempo = false;
    }

    void addMarker(double timeSec, double bpm, int beatNum)
    {
        BeatMarker m;
        m.timeSeconds = std::max(0.0, timeSec);
        m.instantBpm = std::clamp(bpm, 40.0, 250.0);
        m.beatNumber = (beatNum >= 1 && beatNum <= 4) ? beatNum : 1;
        markers.push_back(m);
        recalculateStats();
    }

    bool isVariable() const { return hasVariableTempo; }
    double getAverageBpm() const { return averageBpm; }
    double getMinBpm() const { return minBpm; }
    double getMaxBpm() const { return maxBpm; }
    const std::vector<BeatMarker>& getMarkers() const { return markers; }
    bool isEmpty() const { return markers.empty(); }

    // Obtiene el BPM instantáneo en un segundo dado de la pista (interpolación suave)
    double getBpmAt(double timeSeconds) const
    {
        if (markers.empty())
            return averageBpm;

        if (markers.size() == 1 || timeSeconds <= markers.front().timeSeconds)
            return markers.front().instantBpm;

        if (timeSeconds >= markers.back().timeSeconds)
            return markers.back().instantBpm;

        // Búsqueda binaria del intervalo de tiempo correspondiente
        auto it = std::upper_bound(markers.begin(), markers.end(), timeSeconds,
            [](double t, const BeatMarker& m) { return t < m.timeSeconds; });

        if (it == markers.end())
            return markers.back().instantBpm;

        const auto& next = *it;
        const auto& prev = *(it - 1);
        double span = next.timeSeconds - prev.timeSeconds;
        if (span <= 1e-4)
            return prev.instantBpm;

        double fraction = (timeSeconds - prev.timeSeconds) / span;
        return prev.instantBpm + fraction * (next.instantBpm - prev.instantBpm);
    }

    // Encuentra el pulso más cercano para quantize / snap
    double getClosestBeat(double timeSeconds) const
    {
        if (markers.empty())
            return timeSeconds;

        auto it = std::lower_bound(markers.begin(), markers.end(), timeSeconds,
            [](const BeatMarker& m, double t) { return m.timeSeconds < t; });

        if (it == markers.end())
            return markers.back().timeSeconds;
        if (it == markers.begin())
            return markers.front().timeSeconds;

        double d1 = std::abs((it - 1)->timeSeconds - timeSeconds);
        double d2 = std::abs(it->timeSeconds - timeSeconds);
        return (d1 <= d2) ? (it - 1)->timeSeconds : it->timeSeconds;
    }

private:
    void recalculateStats()
    {
        if (markers.empty())
            return;

        double sumBpm = 0.0;
        minBpm = 999.0;
        maxBpm = 0.0;

        for (const auto& m : markers)
        {
            sumBpm += m.instantBpm;
            minBpm = std::min(minBpm, m.instantBpm);
            maxBpm = std::max(maxBpm, m.instantBpm);
        }

        averageBpm = sumBpm / static_cast<double>(markers.size());
        // Se considera variable si el tempo fluctúa más de 1.8 BPM en el transcurso del tema
        hasVariableTempo = (maxBpm - minBpm) > 1.8;
    }

    std::vector<BeatMarker> markers;
    double averageBpm { 120.0 };
    double minBpm { 120.0 };
    double maxBpm { 120.0 };
    bool hasVariableTempo { false };
};
