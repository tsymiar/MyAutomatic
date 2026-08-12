#include "EchoEngine.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>

EchoEngine::EchoEngine(QObject* parent) : QObject(parent) {
    srand(static_cast<unsigned>(time(nullptr)));
    initCharacters();
    initEndings();
    initMentorMessages();
    initHallucinations();
    initNoiseDialogues();
    initMorseMessages();
}

EchoEngine::~EchoEngine() {}

// ═══════════════════════════════════════════
//  初始化：角色、结局、对话
// ═══════════════════════════════════════════

void EchoEngine::initCharacters() {
    m_characters = {
        {"周宁", "28岁，实验室助理研究员",
         "三年前出过严重车祸，康复后入职，记忆中存在大量空白期。实际上是林薇进行'不完全声纹移植'的实验体——你的原始人格在车祸中早已消亡。"},
        {"林薇", "45岁，神经声学权威，失踪者",
         "发现军方要利用她的技术抹除'不稳定士兵'的人格，于是自行销毁核心数据。但她将'自己的完整人格声纹'移植给了周宁，作为逃逸载体。"},
        {"陈远山", "52岁，实验室负责人，军方背景",
         "林薇的大学同窗，也是出卖她的人。他需要林薇的大脑来解锁加密的核心算法，但他不知道林薇的'备份'就在你体内。"},
        {"老刘", "62岁，门卫，沉默寡言",
         "曾是林薇的第一任实验助手，因目睹一次失败的移植而精神受创，从此不再说话。但他会用摩斯电码敲击桌面——这是你早期解谜的关键信息来源。"},
        {"底噪", "存在于所有音频文件背景中的0.5Hz低频",
         "这是林薇提前植入的'唤醒信号'。当玩家拼出足够多的'错误版本'时，底噪会逐渐增强，最终与你直接对话。"}
    };
    // 角色对话
    m_characterDialogues["林薇"] = QStringList{
        "核心盘我藏好了，你没机会的。",
        "小周，如果你听到这段话，说明我已经不在了。",
        "你现在拼接的每一个声音，都来自你的未来。",
        "你是在给过去的我传递信息。所以，请告诉我：你那边，天亮了吗？",
        "对不起，我只能让你活在我的声音里。",
        "你不是被夺舍的容器，你是我的选择。"
    };
    m_characterDialogues["陈远山"] = QStringList{
        "你藏在一段声音里对吧？那我毁掉所有声音。",
        "小周，我看你门禁卡刷到了地下三层。那里辐射超标，快上来。",
        "你走到哪里，我就把哪里的音频切断。你是一只泡在静水里的耳朵——没有声音，你就是瞎子。"
    };
    m_characterDialogues["底噪"] = QStringList{
        "……你听到了吗？那不是我发出的声音，是你自己在震动。",
        "别害怕低频。它是唯一不会被谎言覆盖的频率。",
        "我已经在这段频率里等了三年。你终于听到了。"
    };
}

void EchoEngine::initEndings() {
    m_endings[EndingType::SilentArchive] = {
        EndingType::SilentArchive, "静默档案",
        "你提交的报告完美无缺，被评为'年度优秀员工'。但最后一幕，你对着镜子微笑时，镜中的你没开口，背景音响起了林薇的尖叫声——你被完全覆盖了，但覆盖你的不是林薇，是陈主任植入的'忠诚声纹'。",
        "全程使用官方碎片，选择信任陈主任"
    };
    m_endings[EndingType::EchoOrphan] = {
        EndingType::EchoOrphan, "回声孤儿",
        "你成功摧毁了实验室所有数据，让陈远山落网。但你失去了一切声音感知能力，永远活在绝对寂静中。结局画面：你坐在海边，看浪花翻涌，但你听不见任何声音。",
        "第三章选择拒绝融合，困难模式通关"
    };
    m_endings[EndingType::Duet] = {
        EndingType::Duet, "双声部",
        "林薇苏醒，指证陈远山。而你作为'残存周宁'只剩数年寿命，但你们两人在最后时光里合作写了一本《声纹伦理学》——结局文本说：'有些声音不必分清是谁的，只要有人听见，它就没死。'",
        "第三章选择接受融合，第四章将声纹转回林薇"
    };
    m_endings[EndingType::Matricide] = {
        EndingType::Matricide, "弑母",
        "你删除林薇肉体的低频录音。她平静死去，密钥消失。你成为了A-Lab新主任，但你每次听到'安静'二字都会剧烈头痛——因为那是她死前说的最后一个词。",
        "第三章选择分离，第四章关闭生命维持"
    };
    m_endings[EndingType::Rewinder] = {
        EndingType::Rewinder, "倒带者",
        "你发现整个游戏的所有关卡在倒放后组成了一段完整的录音：林薇在教你如何把她救出来。你解锁'导师视角'，操控林薇从内部配合自己，达成完美逃生——两人意识最终在服务器云端融合，永不分离。",
        "二周目选择逆向播放贯穿全程"
    };
    m_endings[EndingType::ZeroDecibel] = {
        EndingType::ZeroDecibel, "零分贝",
        "你拼出的画面是：整个A-Lab、陈远山、林薇、甚至周宁，都是某个更高维度'声学模拟程序'中的测试单元。你听到了系统管理员的声音：'第114514次模拟失败，人格分裂度99.8%，建议重启。'然后屏幕出现一行字：'你听到了真相，但你无法被听见。' 游戏强制删除所有存档，回到初始菜单，背景音乐彻底消失。",
        "全周目完成，拼凑出所有红色伪造碎片的集合体"
    };
}

void EchoEngine::initMentorMessages() {
    m_mentorMessages << "别相信你听到的任何声音，包括你自己的。"
                     << "声音可以被雕刻在金属表面，记忆可以被移植进声波里。"
                     << "{name}，如果你听到这段话，说明我已经不在了。"
                     << "你现在拼接的每一个声音，都来自你的未来。"
                     << "你是在给过去的我传递信息。所以，请告诉我：你那边，天亮了吗？"
                     << "{name}，如果你听到这里，说明你已经选择了不信任。很好。接下来你要做的不是拼凑真相，而是拆解谎言——把所有你认为'正确'的音频倒过来放一遍。"
                     << "对不起，我只能让你活在我的声音里。"
                     << "你不是被夺舍的容器，你是我的选择——我放弃了完整逃生的机会，把记忆拆碎混入你的残存意识中，就是为了让你带着我的知识活下去。"
                     << "陈远山要的不是我的人，是我脑中的密钥。只要你活着，他就永远拿不到。";
}

void EchoEngine::initHallucinations() {
    m_hallucinations << "……救我……"
                     << "关掉它，快关掉它！"
                     << "你不是你。"
                     << "听，那声音在你的墙里面。"
                     << "导师就在矩阵里，她在看着你。"
                     << "低频……低频一直在响，你没听到吗？"
                     << "你的心跳声是别人的。"
                     << "滴——那不是设备的声音。"
                     << "镜子里的你没有开口。"
                     << "那个时钟三天前就停了。"
                     << "第114514次模拟……";
}

void EchoEngine::initNoiseDialogues() {
    m_noiseDialogues << "……你听到了吗？"
                     << "别害怕低频。它是唯一不会被谎言覆盖的频率。"
                     << "我已经在这段频率里等了三年。"
                     << "你的心跳和我的声纹已经同步。"
                     << "陈远山在监听所有高频段，但低频他听不到。"
                     << "当你拼出足够多的错误时，我就能说话了。"
                     << "每一段错误拼接，都是我的一小片意识在苏醒。"
                     << "你越偏离官方版本，就越接近我。";
}

void EchoEngine::initMorseMessages() {
    m_morseMessages << "跑"        // ... --- ...
                    << "别信他"    // 综合
                    << "地下"      // 综合
                    << "U盘"       // 综合
                    << "坐标"      // 综合
                    << "她还活着"; // 综合
}

// ═══════════════════════════════════════════
//  碎片生成
// ═══════════════════════════════════════════

void EchoEngine::initFragmentTypes() {
    m_fragments.clear();
    m_timeline.clear();
    m_nextId = 0;
    m_isDistorted = false;
    m_silenceTriggered = false;
    m_currentAnchor = AnchorType::None;
    int slotCount = 6 + static_cast<int>(m_currentChapter) * 2;
    if (m_currentChapter >= GameChapter::Chapter4) slotCount = 12;
    for (int i = 0; i < slotCount; ++i) {
        m_timeline.push_back({i, -1, false, false, AnchorType::None});
    }
}

SoundFragment EchoEngine::createFragment(FragmentType type, Credibility cred, float duration,
                                           bool isAnchor, AnchorType anchor) {
    SoundFragment frag;
    frag.type = type;
    frag.credibility = cred;
    frag.durationSec = duration;
    frag.id = m_nextId++;
    frag.isPlaced = false;
    frag.timelineSlot = -1;
    frag.isAnchor = isAnchor;
    frag.anchorType = anchor;

    auto setProps = [&](const QString& n, const QString& d, float f, float a) {
        frag.name = n; frag.description = d; frag.frequencyHz = f; frag.amplitude = a;
    };
    switch (type) {
    case FragmentType::Footstep:      setProps("脚步声","走廊里渐近的脚步声",200+(rand()%800),0.3f); break;
    case FragmentType::ElectricBuzz:  setProps("电流声","设备电流嗡鸣",50+(rand()%150),0.15f); break;
    case FragmentType::WaterDrop:     setProps("滴水声","水管深处滴水",400+(rand()%600),0.2f); break;
    case FragmentType::Heartbeat:     setProps("心跳声","低沉不规律的心跳",60+(rand()%40),0.5f); break;
    case FragmentType::VoiceWhisper:  setProps("低语声","无法辨认的低语",300+(rand()%1200),0.25f); break;
    case FragmentType::StaticNoise:   setProps("白噪声","持续背景噪声",1000+(rand()%4000),0.1f); break;
    case FragmentType::DoorCreak:     setProps("门吱呀声","沉重铁门推开",800+(rand()%2000),0.4f); break;
    case FragmentType::Typewriter:    setProps("打字机声","老式打字机敲击",500+(rand()%1000),0.35f); break;
    case FragmentType::Breath:        setProps("呼吸声","缓慢沉重的呼吸",100+(rand()%200),0.2f); break;
    case FragmentType::LowFrequency:  setProps("低频嗡鸣","超低频振动",20+(rand()%30),0.08f); break;
    case FragmentType::MetalExpansion:setProps("金属膨胀","金属热胀冷缩",300+(rand()%500),0.15f); break;
    case FragmentType::Geomagnetic:   setProps("地磁波动","微弱地磁波动",5+(rand()%15),0.05f); break;
    case FragmentType::ClockTick:     setProps("时钟滴答","规律滴答声",1000+(rand()%500),0.3f); break;
    case FragmentType::TireScreech:   setProps("轮胎摩擦","刺耳的急刹声",2000+(rand()%3000),0.6f); break;
    case FragmentType::RainAmbient:   setProps("雨声","持续的雨声",300+(rand()%800),0.25f); break;
    default: break;
    }
    // 锚点音特殊描述
    if (isAnchor) {
        switch (anchor) {
        case AnchorType::DoorBeep: frag.name="门禁刷卡声"; frag.description="实验室门禁的电子蜂鸣"; break;
        case AnchorType::CoffeeMachine: frag.name="咖啡机启动声"; frag.description="休息区咖啡机启动"; break;
        case AnchorType::HeartMonitor: frag.name="心率监护仪"; frag.description="规律的心率监护仪蜂鸣"; break;
        case AnchorType::ClockChime: frag.name="钟声"; frag.description="办公室老式挂钟整点报时"; break;
        case AnchorType::KeyTurn: frag.name="钥匙转动"; frag.description="金属钥匙在锁孔中转动"; break;
        default: break;
        }
    }
    // 频谱采样
    int sampleCount = 32;
    frag.spectrumSamples.resize(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        float baseFreq = frag.frequencyHz * (1.0f + i * 0.15f);
        float noise = (rand()%100-50)/100.0f*0.2f;
        float credMod = 1.0f;
        if (cred==Credibility::Fake) credMod=0.3f+(rand()%40)/100.0f;
        if (cred==Credibility::Noise) credMod=0.5f+(rand()%30)/100.0f;
        frag.spectrumSamples[i]=frag.amplitude*credMod*
            expf(-(baseFreq-frag.frequencyHz)*(baseFreq-frag.frequencyHz)
                 /(2.0f*frag.frequencyHz*frag.frequencyHz*0.01f))+noise;
    }
    // 逆向播放秘密
    if (type==FragmentType::TireScreech) {
        frag.revealsSecret=true;
        frag.reverseDescription="轮胎摩擦声倒放后变成了林薇的哭声：'对不起，我只能让你活在我的声音里。'";
    }
    if (type==FragmentType::Heartbeat) {
        frag.revealsSecret=true;
        frag.reverseDescription="心跳声倒放后是一段加密坐标——指向城郊废弃精神病院。";
    }
    return frag;
}

void EchoEngine::generateFragments(GameChapter chapter) {
    m_currentChapter = chapter;
    initFragmentTypes();
    switch (chapter) {
    case GameChapter::Prologue: setupChapterPrologue(); break;
    case GameChapter::Chapter1: setupChapter1(); break;
    case GameChapter::Chapter2: setupChapter2(); break;
    case GameChapter::Chapter3: setupChapter3(); break;
    case GameChapter::Chapter4: setupChapter4(); break;
    case GameChapter::Finale:   setupFinale(); break;
    }
    // 打乱碎片
    for (int i=static_cast<int>(m_fragments.size())-1; i>0; --i) {
        int j=rand()%(i+1);
        std::swap(m_fragments[i],m_fragments[j]);
    }
}

// ═══════════════════════════════════════════
//  各章节碎片配置
// ═══════════════════════════════════════════

void EchoEngine::setupChapterPrologue() {
    m_currentAnchor = AnchorType::DoorBeep;
    // 设置锚点槽位
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::DoorBeep;
    }
    // 锚点音碎片（官方版本缺少惊恐呼吸声）
    m_fragments.push_back(createFragment(FragmentType::DoorCreak, Credibility::Real, 1.5f, true, AnchorType::DoorBeep));
    // 常规碎片
    m_fragments.push_back(createFragment(FragmentType::Footstep, Credibility::Real, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::VoiceWhisper, Credibility::Real, 3.0f));
    m_fragments.push_back(createFragment(FragmentType::Typewriter, Credibility::Real, 2.5f));
    m_fragments.push_back(createFragment(FragmentType::ElectricBuzz, Credibility::Suspicious, 1.8f));
    m_fragments.push_back(createFragment(FragmentType::Breath, Credibility::Suspicious, 2.2f)); // 林薇的惊恐呼吸
    m_fragments.push_back(createFragment(FragmentType::StaticNoise, Credibility::Noise, 1.0f));
    m_fragments.push_back(createFragment(FragmentType::ClockTick, Credibility::Noise, 1.5f));
    m_fragments.push_back(createFragment(FragmentType::LowFrequency, Credibility::Fake, 2.0f));
    // 额外碎片
    for (int i=0; i<8; ++i) {
        FragmentType t=static_cast<FragmentType>(rand()%static_cast<int>(FragmentType::Count));
        m_fragments.push_back(createFragment(t, Credibility::Noise, 1.0f+(rand()%20)/10.0f));
    }
    activateMorse();
}

void EchoEngine::setupChapter1() {
    m_currentAnchor = AnchorType::CoffeeMachine;
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::CoffeeMachine;
    }
    m_fragments.push_back(createFragment(FragmentType::ElectricBuzz, Credibility::Real, 2.0f, true, AnchorType::CoffeeMachine));
    m_fragments.push_back(createFragment(FragmentType::VoiceWhisper, Credibility::Real, 3.5f));
    m_fragments.push_back(createFragment(FragmentType::Footstep, Credibility::Real, 2.5f));
    m_fragments.push_back(createFragment(FragmentType::Typewriter, Credibility::Real, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::DoorCreak, Credibility::Suspicious, 1.5f));
    m_fragments.push_back(createFragment(FragmentType::Breath, Credibility::Suspicious, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::Heartbeat, Credibility::Suspicious, 3.0f));
    m_fragments.push_back(createFragment(FragmentType::StaticNoise, Credibility::Noise, 1.5f));
    m_fragments.push_back(createFragment(FragmentType::LowFrequency, Credibility::Fake, 2.5f));
    m_fragments.push_back(createFragment(FragmentType::ClockTick, Credibility::Fake, 1.0f));
    for (int i=0; i<12; ++i) {
        FragmentType t=static_cast<FragmentType>(rand()%static_cast<int>(FragmentType::Count));
        Credibility c = (rand()%100<40) ? Credibility::Suspicious : Credibility::Noise;
        m_fragments.push_back(createFragment(t, c, 1.0f+(rand()%30)/10.0f));
    }
    // 第一章结束后解锁逆向播放
    unlockReverse();
}

void EchoEngine::setupChapter2() {
    m_currentAnchor = AnchorType::HeartMonitor;
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::HeartMonitor;
    }
    m_fragments.push_back(createFragment(FragmentType::Heartbeat, Credibility::Real, 3.0f, true, AnchorType::HeartMonitor));
    m_fragments.push_back(createFragment(FragmentType::MetalExpansion, Credibility::Real, 2.5f));
    m_fragments.push_back(createFragment(FragmentType::Geomagnetic, Credibility::Real, 4.0f));
    m_fragments.push_back(createFragment(FragmentType::WaterDrop, Credibility::Real, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::VoiceWhisper, Credibility::Suspicious, 3.5f));
    m_fragments.push_back(createFragment(FragmentType::Breath, Credibility::Suspicious, 2.8f));
    m_fragments.push_back(createFragment(FragmentType::DoorCreak, Credibility::Suspicious, 1.8f));
    m_fragments.push_back(createFragment(FragmentType::ElectricBuzz, Credibility::Noise, 1.5f));
    m_fragments.push_back(createFragment(FragmentType::StaticNoise, Credibility::Noise, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::LowFrequency, Credibility::Fake, 3.0f));
    m_fragments.push_back(createFragment(FragmentType::TireScreech, Credibility::Fake, 1.5f));
    for (int i=0; i<14; ++i) {
        FragmentType t=static_cast<FragmentType>(rand()%static_cast<int>(FragmentType::Count));
        Credibility c = (rand()%100<30)?Credibility::Real:(rand()%100<50)?Credibility::Suspicious:Credibility::Noise;
        m_fragments.push_back(createFragment(t, c, 1.0f+(rand()%40)/10.0f));
    }
    increaseLowFreq();
}

void EchoEngine::setupChapter3() {
    m_currentAnchor = AnchorType::KeyTurn;
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::KeyTurn;
    }
    // 自我声纹拼接——碎片是两个版本的人生
    m_fragments.push_back(createFragment(FragmentType::DoorCreak, Credibility::Real, 2.0f, true, AnchorType::KeyTurn));
    m_fragments.push_back(createFragment(FragmentType::RainAmbient, Credibility::Real, 3.5f));    // 周宁的童年雨声
    m_fragments.push_back(createFragment(FragmentType::Typewriter, Credibility::Real, 2.0f));     // 林薇决定做声纹研究
    m_fragments.push_back(createFragment(FragmentType::Heartbeat, Credibility::Real, 3.0f));       // 重叠心跳
    m_fragments.push_back(createFragment(FragmentType::TireScreech, Credibility::Suspicious, 2.0f));// 车祸
    m_fragments.push_back(createFragment(FragmentType::VoiceWhisper, Credibility::Suspicious, 4.0f));
    m_fragments.push_back(createFragment(FragmentType::Breath, Credibility::Suspicious, 2.5f));
    m_fragments.push_back(createFragment(FragmentType::ElectricBuzz, Credibility::Noise, 1.5f));
    m_fragments.push_back(createFragment(FragmentType::StaticNoise, Credibility::Noise, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::LowFrequency, Credibility::Fake, 3.5f));
    m_fragments.push_back(createFragment(FragmentType::ClockTick, Credibility::Fake, 1.0f));
    for (int i=0; i<16; ++i) {
        FragmentType t=static_cast<FragmentType>(rand()%static_cast<int>(FragmentType::Count));
        Credibility c = (rand()%100<20)?Credibility::Real:(rand()%100<40)?Credibility::Suspicious:Credibility::Fake;
        m_fragments.push_back(createFragment(t, c, 1.0f+(rand()%40)/10.0f));
    }
    increaseLowFreq();
    increaseLowFreq();
}

void EchoEngine::setupChapter4() {
    m_currentAnchor = AnchorType::HeartMonitor;
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::HeartMonitor;
    }
    m_fragments.push_back(createFragment(FragmentType::Heartbeat, Credibility::Real, 4.0f, true, AnchorType::HeartMonitor));
    m_fragments.push_back(createFragment(FragmentType::LowFrequency, Credibility::Real, 5.0f));
    m_fragments.push_back(createFragment(FragmentType::Breath, Credibility::Real, 3.5f));
    m_fragments.push_back(createFragment(FragmentType::VoiceWhisper, Credibility::Real, 4.5f));
    m_fragments.push_back(createFragment(FragmentType::ElectricBuzz, Credibility::Suspicious, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::DoorCreak, Credibility::Suspicious, 1.5f));
    m_fragments.push_back(createFragment(FragmentType::MetalExpansion, Credibility::Noise, 2.5f));
    m_fragments.push_back(createFragment(FragmentType::StaticNoise, Credibility::Noise, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::Geomagnetic, Credibility::Fake, 3.0f));
    m_fragments.push_back(createFragment(FragmentType::TireScreech, Credibility::Fake, 2.0f));
    for (int i=0; i<18; ++i) {
        FragmentType t=static_cast<FragmentType>(rand()%static_cast<int>(FragmentType::Count));
        Credibility c = static_cast<Credibility>(rand()%4);
        m_fragments.push_back(createFragment(t, c, 1.0f+(rand()%40)/10.0f));
    }
    increaseLowFreq();
    increaseLowFreq();
}

void EchoEngine::setupFinale() {
    m_currentAnchor = AnchorType::ClockChime;
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::ClockChime;
    }
    for (int i=0; i<24; ++i) {
        FragmentType t=static_cast<FragmentType>(rand()%static_cast<int>(FragmentType::Count));
        Credibility c = static_cast<Credibility>(rand()%4);
        m_fragments.push_back(createFragment(t, c, 1.0f+(rand()%40)/10.0f));
    }
    increaseLowFreq(); increaseLowFreq(); increaseLowFreq();
}

// ═══════════════════════════════════════════
//  时间轴操作
// ═══════════════════════════════════════════

bool EchoEngine::placeFragment(int fragmentId, int slotIndex) {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_timeline.size())) return false;
    SoundFragment* frag = nullptr;
    for (auto& f : m_fragments) { if (f.id == fragmentId) { frag = &f; break; } }
    if (!frag) return false;

    // 内联替换逻辑，避免 removeFragment 的中间信号闪烁
    int oldFragId = m_timeline[slotIndex].fragmentId;
    if (oldFragId >= 0) {
        // 清除旧碎片在槽位中的引用
        for (auto& f : m_fragments) {
            if (f.id == oldFragId) { f.isPlaced = false; f.timelineSlot = -1; break; }
        }
        emit fragmentRemoved(oldFragId, slotIndex);
    }
    if (frag->isPlaced && frag->timelineSlot >= 0) {
        m_timeline[frag->timelineSlot].fragmentId = -1;
    }
    m_timeline[slotIndex].fragmentId = fragmentId;
    frag->isPlaced = true;
    frag->timelineSlot = slotIndex;

    float coherence = calculateCoherence();
    m_isDistorted = (coherence < 0.4f);
    if (frag->credibility == Credibility::Fake || frag->credibility == Credibility::Noise) {
        adjustParanoia(0.05f); increaseLowFreq();
    } else if (frag->credibility == Credibility::Real) {
        adjustParanoia(-0.02f);
    }
    adjustAnxiety(0.02f);
    emit fragmentPlaced(fragmentId, slotIndex);
    if (isTimelineFull()) {
        emit sceneCompleted(static_cast<int>(m_currentChapter), m_isDistorted);
    }
    return true;
}

bool EchoEngine::removeFragment(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_timeline.size())) return false;
    int fragId = m_timeline[slotIndex].fragmentId;
    if (fragId < 0) return false;
    m_timeline[slotIndex].fragmentId = -1;
    for (auto& f : m_fragments) {
        if (f.id == fragId) { f.isPlaced = false; f.timelineSlot = -1; break; }
    }
    m_isDistorted = false;
    adjustAnxiety(-0.01f);
    emit fragmentRemoved(fragId, slotIndex);
    return true;
}

float EchoEngine::calculateCoherence() {
    if (placedFragmentCount() < 2) return 1.0f;
    float totalScore = 0.0f; int pairCount = 0;
    Credibility prevCred = Credibility::Real; bool first = true;
    for (const auto& slot : m_timeline) {
        if (slot.fragmentId < 0) continue;
        for (const auto& f : m_fragments) {
            if (f.id == slot.fragmentId) {
                if (!first) {
                    if (f.credibility == prevCred) totalScore += 1.0f;
                    else if (abs(static_cast<int>(f.credibility)-static_cast<int>(prevCred))==1) totalScore += 0.5f;
                    pairCount++;
                }
                first = false; prevCred = f.credibility; break;
            }
        }
    }
    float paranoiaMod = 1.0f - m_paranoiaLevel * 0.5f;
    if (m_hardMode) paranoiaMod *= 0.7f;
    if (pairCount == 0) return 1.0f;
    return (totalScore / pairCount) * paranoiaMod;
}

void EchoEngine::adjustParanoia(float delta) {
    m_paranoiaLevel += delta;
    if (m_paranoiaLevel < 0.0f) m_paranoiaLevel = 0.0f;
    if (m_paranoiaLevel > 1.0f) m_paranoiaLevel = 1.0f;
    emit paranoiaChanged(m_paranoiaLevel);
    if (m_paranoiaLevel > 0.7f && (rand()%100) < 30) {
        emit hallucinationTriggered(m_hallucinations[rand()%m_hallucinations.size()]);
    }
}

void EchoEngine::adjustAnxiety(float delta) {
    m_anxietyLevel += delta;
    if (m_anxietyLevel < 0.0f) m_anxietyLevel = 0.0f;
    if (m_anxietyLevel > 1.0f) m_anxietyLevel = 1.0f;
    emit anxietyChanged(m_anxietyLevel);
}

int EchoEngine::placedFragmentCount() const {
    int c=0; for(auto& s:m_timeline) if(s.fragmentId>=0) c++; return c;
}

bool EchoEngine::isTimelineFull() const {
    for(auto& s:m_timeline) if(s.fragmentId<0) return false; return true;
}

void EchoEngine::resetCurrentChapter() {
    generateFragments(m_currentChapter);
    m_paranoiaLevel=0.3f; m_anxietyLevel=0.1f;
}

// ═══════════════════════════════════════════
//  锚点音
// ═══════════════════════════════════════════

bool EchoEngine::isAnchorCorrect() const {
    if (m_timeline.empty() || !m_timeline[0].isAnchorSlot) return true;
    int fid = m_timeline[0].fragmentId;
    if (fid < 0) return false;
    for (auto& f : m_fragments)
        if (f.id == fid) return f.isAnchor && f.anchorType == m_timeline[0].requiredAnchor;
    return false;
}

// ═══════════════════════════════════════════
//  逆向播放
// ═══════════════════════════════════════════

void EchoEngine::unlockReverse() {
    if (!m_reverseUnlocked) {
        m_reverseUnlocked = true;
        emit reverseUnlocked();
    }
}

void EchoEngine::reverseFragment(int fragmentId) {
    for (auto& f : m_fragments) {
        if (f.id == fragmentId) { f.isReversed = true; break; }
    }
}

bool EchoEngine::isFragmentReversed(int fragmentId) const {
    for (auto& f : m_fragments) if (f.id == fragmentId) return f.isReversed;
    return false;
}

QString EchoEngine::getReverseDescription(int fragmentId) const {
    for (auto& f : m_fragments)
        if (f.id == fragmentId && f.revealsSecret) return f.reverseDescription;
    return "";
}

// ═══════════════════════════════════════════
//  导师/幻觉/底噪/摩斯
// ═══════════════════════════════════════════

QString EchoEngine::getMentorMessage(int index) const {
    if (index<0||index>=m_mentorMessages.size()) return "";
    QString msg=m_mentorMessages[index];
    QString name=m_playerName.isEmpty()?"小周":m_playerName;
    msg.replace("{name}",name);
    return msg;
}

QString EchoEngine::getHallucinationText() const {
    if (m_paranoiaLevel<0.5f) return "";
    return m_hallucinations[rand()%m_hallucinations.size()];
}

QString EchoEngine::getLowFreqWarning() const {
    if (m_paranoiaLevel>0.6f) return "警告：检测到持续低频信号，来源不明。";
    return "";
}

void EchoEngine::increaseLowFreq() {
    m_lowFreqIntensity += 0.03f;
    if (m_lowFreqIntensity > 1.0f) m_lowFreqIntensity = 1.0f;
    if (m_lowFreqIntensity > 0.5f && (rand()%100)<15) {
        emit noiseCommunion(m_noiseDialogues[rand()%m_noiseDialogues.size()]);
    }
    // 极高底噪触发静默模式
    if (m_lowFreqIntensity > 0.85f && !m_silenceTriggered) {
        m_silenceTriggered = true;
        emit silenceMode(true);
    }
}

QString EchoEngine::noiseDialogue() const {
    if (m_lowFreqIntensity<0.5f) return "";
    return m_noiseDialogues[rand()%m_noiseDialogues.size()];
}

QString EchoEngine::getMorseMessage() const {
    if (!m_morseActive) return "";
    return m_morseMessages[rand()%m_morseMessages.size()];
}

QString EchoEngine::getCharacterDialogue(const QString& name, int lineIndex) const {
    auto it = m_characterDialogues.find(name);
    if (it == m_characterDialogues.constEnd()) return "";
    const auto& lines = it.value();
    if (lineIndex<0||lineIndex>=lines.size()) return "";
    return lines[lineIndex];
}

void EchoEngine::revealCharacter(int index) {
    if (index>=0 && index<static_cast<int>(m_characters.size()))
        m_characters[index].isRevealed = true;
}

// ═══════════════════════════════════════════
//  选择系统
// ═══════════════════════════════════════════

void EchoEngine::recordChoice(PlayerChoice choice) {
    m_choices.push_back({choice, m_currentChapter, ""});
    if (choice == PlayerChoice::RejectFusion) setHardMode(true);
}

bool EchoEngine::hasChosen(PlayerChoice choice) const {
    for (auto& c : m_choices) if (c.choice == choice) return true;
    return false;
}

PlayerChoice EchoEngine::getChapterChoice(GameChapter chapter) const {
    for (auto& c : m_choices) if (c.chapter == chapter) return c.choice;
    return PlayerChoice::None;
}

// ═══════════════════════════════════════════
//  结局系统
// ═══════════════════════════════════════════

EndingType EchoEngine::calculateEnding() const {
    bool trustDirector = !hasChosen(PlayerChoice::RecordSigh);
    bool useOfficial = m_useOfficialAnchor;
    auto ch3 = getChapterChoice(GameChapter::Chapter3);
    auto ch4 = getChapterChoice(GameChapter::Chapter4);
    int fakeCount = 0;
    for (auto& f : m_fragments) if (f.credibility==Credibility::Fake && f.isPlaced) fakeCount++;
    int reversedCount = 0;
    for (auto& f : m_fragments) if (f.isReversed) reversedCount++;

    if (fakeCount >= 10 && m_lowFreqIntensity > 0.9f) return EndingType::ZeroDecibel;
    if (reversedCount >= 8) return EndingType::Rewinder;
    if (ch3 == PlayerChoice::AcceptFusion && ch4 == PlayerChoice::TransferBack) return EndingType::Duet;
    if (ch3 == PlayerChoice::SeparateVoices && ch4 == PlayerChoice::DeleteLifeSupport) return EndingType::Matricide;
    if (ch3 == PlayerChoice::RejectFusion) return EndingType::EchoOrphan;
    if (trustDirector || useOfficial) return EndingType::SilentArchive;
    return EndingType::Duet;
}

EndingInfo EchoEngine::getEndingInfo(EndingType type) const {
    return m_endings.value(type, {EndingType::None,"未知","","",false});
}

int EchoEngine::unlockedEndingCount() const {
    int c=0; for(auto it=m_endings.begin();it!=m_endings.end();++it) if(it->unlocked) c++; return c;
}

QString EchoEngine::endingTitle() const { return getEndingInfo(calculateEnding()).title; }
QString EchoEngine::endingDescription() const { return getEndingInfo(calculateEnding()).description; }
bool EchoEngine::isEndingUnlocked(EndingType type) const { return m_endings.value(type,{}).unlocked; }

// ═══════════════════════════════════════════
//  章节信息
// ═══════════════════════════════════════════

QString EchoEngine::chapterName() const {
    switch (m_currentChapter) {
    case GameChapter::Prologue: return "序章：入职第7天";
    case GameChapter::Chapter1: return "第一章：日常的裂隙";
    case GameChapter::Chapter2: return "第二章：地下三十米";
    case GameChapter::Chapter3: return "第三章：镜像之家";
    case GameChapter::Chapter4: return "第四章：声牢";
    case GameChapter::Finale:   return "终章";
    }
    return "未知";
}

QString EchoEngine::chapterDescription() const {
    switch (m_currentChapter) {
    case GameChapter::Prologue:
        return "你的工位，耳机里循环播放林薇留下的'实验室安全须知'录音。\n学习抓取环境音、识别可信度，拼接昨天下午茶水间的对话。";
    case GameChapter::Chapter1:
        return "按陈主任的要求，整理林薇失踪前72小时的所有工作录音，形成'官方报告'。\n但那些'官方碎片'里藏着被删改的痕迹……";
    case GameChapter::Chapter2:
        return "潜入实验室地下废弃档案库，温度-4℃。这里的碎片不再是对话，而是纯物理振动声——金属膨胀、地磁波动、心跳共振。\n将物理声波与人体器官频率匹配，聆听被封印的历史。";
    case GameChapter::Chapter3:
        return "你回到家，所有智能设备都在播放同一段白噪音。\n这一次，你要拼接的不是外界声音——而是你自己的记忆。\n但你脑海中的碎片存在两个版本……";
    case GameChapter::Chapter4:
        return "废弃精神病院地下三层。陈远山已封锁出口，他在切断所有音频。\n你的声纹重构仪现在可以主动发射频率——粉碎墙壁、模仿人声。\n但每一次发射都会增加你的声纹残留……";
    case GameChapter::Finale:
        return "最后的真相就在眼前。你拼凑的不是案件，而是你自己被篡改过的人生。";
    }
    return "";
}

QColor EchoEngine::credibilityToSpectrumColor(Credibility c) {
    switch (c) {
    case Credibility::Real: return QColor(0,180,255);
    case Credibility::Suspicious: return QColor(0,220,200);
    case Credibility::Noise: return QColor(255,200,0);
    case Credibility::Fake: return QColor(255,40,40);
    }
    return Qt::white;
}
