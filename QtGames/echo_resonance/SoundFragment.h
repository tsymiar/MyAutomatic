#pragma once
#include <QString>
#include <QColor>
#include <vector>

// 声音碎片类型
enum class FragmentType {
    Footstep, ElectricBuzz, WaterDrop, Heartbeat,
    VoiceWhisper, StaticNoise, DoorCreak, Typewriter,
    Breath, LowFrequency, Count
};

// 可信度等级
enum class Credibility {
    Real = 0, Suspicious, Noise, Fake
};

// 单个声音碎片
struct SoundFragment {
    FragmentType type = FragmentType::Footstep;
    QString name;
    QString description;
    Credibility credibility = Credibility::Real;
    float durationSec = 1.0f;
    float frequencyHz = 440.0f;
    float amplitude = 0.5f;
    bool isPlaced = false;
    int timelineSlot = -1;
    int id = -1;
    std::vector<float> spectrumSamples;

    QColor displayColor() const;
    QString credibilityLabel() const;
};

// 时间轴槽位
struct TimelineSlot {
    int index = 0;
    int fragmentId = -1;
    bool isLocked = false;
};
