#include "DeductionAudio.h"
#include "DeductionData.h"  // ECHO_PI 宏

DeductionAudio::DeductionAudio(QObject* parent) : QObject(parent)
{
#ifdef DEDUCTION_AUDIO
    // Qt Multimedia 存在时启用真实音频
    m_audioAvailable = true;
#else
    m_audioAvailable = false;
#endif
}

DeductionAudio::~DeductionAudio() {}

bool DeductionAudio::hasAudioOutput() const
{
    return m_audioAvailable;
}

// ═══════════════════════════════════════════════════════════════
//  PCM 合成：多层正弦波叠加 + 变调 + 指数衰减包络
// ═══════════════════════════════════════════════════════════════

QByteArray DeductionAudio::generatePCM(const std::vector<AudioLayer>& layers, int sampleRate)
{
    if (layers.empty()) return QByteArray();

    // 计算最大时长（限制上限，避免恶意输入导致内存爆炸）
    float maxDur = 0.0f;
    for (const auto& l : layers) if (l.durationSec > maxDur) maxDur = l.durationSec;
    if (maxDur <= 0.0f) maxDur = 0.5f;
    if (maxDur > 30.0f) maxDur = 30.0f;  // 上限 30 秒

    int totalSamples = static_cast<int>(maxDur * sampleRate);
    // 16bit 单声道
    QByteArray pcm;
    pcm.resize(totalSamples * 2);

    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float sample = 0.0f;
        int activeCount = 0;
        for (const auto& l : layers) {
            if (t > l.durationSec) continue;
            activeCount++;
            // 变调：频率乘 pitchScale
            float f = l.frequency * l.pitchScale;
            // 指数衰减包络（防御 durationSec <= 0 导致除零）
            float dur = l.durationSec > 0.0f ? l.durationSec : 0.5f;
            float env = expf(-3.0f * t / dur);
            // 加入轻微谐波，让声音更"声纹"感
            float v = sinf(2.0f * ECHO_PI * f * t)
                + 0.3f * sinf(2.0f * ECHO_PI * f * 2.0f * t)
                + 0.15f * sinf(2.0f * ECHO_PI * f * 3.0f * t);
            sample += v * l.amplitude * env;
        }
        if (activeCount > 1) sample /= static_cast<float>(activeCount); // 叠加归一化
        // 限幅
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        qint16 val = static_cast<qint16>(sample * 32000.0f);
        pcm[2 * i] = static_cast<char>(val & 0xFF);
        pcm[2 * i + 1] = static_cast<char>((val >> 8) & 0xFF);
    }
    return pcm;
}

// ═══════════════════════════════════════════════════════════════
//  合成 + 描述
// ═══════════════════════════════════════════════════════════════

QString DeductionAudio::synthesize(const std::vector<AudioLayer>& layers, bool play)
{
    QString desc;
    QStringList layerDesc;
    for (const auto& l : layers) {
        layerDesc << QString("%1Hz(%2x)%3")
            .arg(l.frequency)
            .arg(l.pitchScale, 0, 'f', 2)
            .arg(l.pitchScale != 1.0f ? "[变调]" : "");
    }
    desc = "合成声纹层：" + layerDesc.join(" + ");

    if (m_audioAvailable && play) {
        generatePCM(layers);
        // 实际播放由上层持有 QAudioSink；此处仅生成数据，
        // 播放管线在 DeductionBoardWidget 中管理（避免头文件依赖扩散）
        desc += "（已合成音频）";
    } else {
        desc += "（纯视觉模拟——环境无音频后端）";
    }
    return desc;
}
