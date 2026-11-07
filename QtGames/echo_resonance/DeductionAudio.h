#pragma once
#include <QObject>
#include <QString>
#include <vector>
#include <QByteArray>
#include <cmath>

// 推演音频合成器：实时合成推演音频（变调 + 叠加）
// 目标环境缺少 Qt Multimedia 时，退化为纯参数描述（不播放声音）
// DEDUCTION_AUDIO 由 .pro 按 qtHaveModule(multimedia) 自动定义
// 本头文件只声明接口，不依赖 Qt Multimedia 的具体类，
// 以便同时兼容 Qt5（QAudioOutput）与 Qt6（QAudioSink）。

// 单一声纹层（用于叠加合成）
struct AudioLayer {
    float frequency;    // 基频 Hz
    float amplitude;    // 0.0-1.0
    float durationSec;  // 时长
    float pitchScale;   // 变调系数（1.0 原始，1.2 = 升高约3个半音）
};

class DeductionAudio : public QObject {
    Q_OBJECT
public:
    explicit DeductionAudio(QObject* parent = nullptr);
    ~DeductionAudio();

    // 是否有真实音频输出能力
    bool hasAudioOutput() const;

    // 合成一段推演音频（多层叠加 + 变调），返回描述文字
    // 若支持真实音频则同时播放
    QString synthesize(const std::vector<AudioLayer>& layers, bool play = true);

    // 生成 PCM 数据（16bit 单声道 44100Hz），供真实播放或后续扩展
    QByteArray generatePCM(const std::vector<AudioLayer>& layers, int sampleRate = 44100);

private:
    bool m_audioAvailable = false;
};
