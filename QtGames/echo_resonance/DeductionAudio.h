#pragma once
#include <QObject>
#include <QString>
#include <vector>
#include <QByteArray>

// 推演音频合成器（变调 + 叠加）；缺 Qt Multimedia 时退化为纯参数描述。
// DEDUCTION_AUDIO 由 .pro 按 qtHaveModule(multimedia) 定义；头文件不依赖具体类，兼容 Qt5/Qt6。

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

    // 是否有真实音频输出能力（否则推演盘标注"视觉模拟"）
    bool hasAudioOutput() const;

    // 合成推演音频（多层叠加 + 变调）并返回描述文字；支持真实音频时同时播放。
    QString synthesize(const std::vector<AudioLayer>& layers, bool play = true);

    // 生成 PCM 数据（16bit 单声道 44100Hz），供真实播放或后续扩展
    QByteArray generatePCM(const std::vector<AudioLayer>& layers, int sampleRate = 44100);

private:
    bool m_audioAvailable = false;
};
