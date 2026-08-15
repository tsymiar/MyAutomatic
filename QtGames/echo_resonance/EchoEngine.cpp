#include "EchoEngine.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>

EchoEngine::EchoEngine(QObject* parent) : QObject(parent)
{
    srand(static_cast<unsigned>(time(nullptr)));
    initCharacters();
    initEndings();
    initMentorMessages();
    initHallucinations();
    initNoiseDialogues();
    initMorseMessages();
    initEchoFragments();
    initEpilogues();
}

EchoEngine::~EchoEngine() {}

// ═══════════════════════════════════════════
//  初始化：角色、结局、对话
// ═══════════════════════════════════════════

void EchoEngine::initCharacters()
{
    m_characters = {
        {"{name}", "28岁，实验室助理研究员",
         "14岁时在一场车祸中被困变形车厢，林薇录下你敲击车门的呼救声精准定位救了你。你始终以为她是恩师，其实她把你当作'失散多年的回声'——你敲击的节奏与她母亲临终前心跳仪波形完全一致。"},
        {"林薇", "45岁，神经声学权威，失踪者",
         "发现军方要利用她的技术抹除'不稳定士兵'的人格，于是自行销毁核心数据。但她将'自己的完整人格声纹'移植给了{name}，作为逃逸载体。她救过你两次：一次在车祸现场，一次用她的生命。"},
        {"陈远山", "52岁，实验室负责人，军方背景，右耳先天失聪",
         "他年轻时是林薇最默契的搭档，但右耳完全失聪。他不恨林薇，他恨的是'声音有选择权'。当年他主张军事化开发，是因为军方答应治好他的耳朵，代价是交出所有成果。林薇的拒绝，在他眼中不是道德坚守，而是断了他唯一的希望。"},
        {"老刘", "62岁，门卫，沉默寡言",
         "林薇最早的实验助手，参与过第一次'人格声纹移植'——被试者是他的亲弟弟。移植失败后弟弟人格分裂，深夜砸碎所有录音设备后跳楼。老刘从此不再说话，自愿降职为门卫，终身守护地下档案库入口——不是为了保护秘密，是为了不让任何人再打开那扇门。"},
        {"何悦", "22岁，实习生，你的名义带教对象",
         "刚入职两周，天真话多，喜欢在工位哼歌。她的声音天然干净，不带任何信息污染，是唯一可以当作'绝对参考系'的音源。陈远山故意安排她坐你隔壁，用她的碎碎念掩盖异常低频——但她无意中录下了陈远山打给军方的加密电话。"},
        {"底噪", "存在于所有音频文件背景中的0.5Hz低频",
         "这是林薇提前植入的'唤醒信号'。当玩家拼出足够多的'错误版本'时，底噪会逐渐增强，最终与你直接对话。"}
    };
    // 角色对话
    m_characterDialogues["林薇"] = QStringList{
        "核心盘我藏好了，你没机会的。",
        "{name}，如果你听到这段话，说明我已经不在了。",
        "你现在拼接的每一个声音，都来自你的未来。",
        "你是在给过去的我传递信息。所以，请告诉我：你那边，天亮了吗？",
        "对不起，我只能让你活在我的声音里。",
        "你不是被夺舍的容器，你是我的选择。",
        "孩子别怕，阿姨听得到你，阿姨这辈子都听得到你。",  // 14年前救援录音
        "欢迎回家。这里所有墙壁里都藏着我为你准备的备用碎片。如果有一天你连自己都不敢信了，就对着镜子说话，我会在镜子的另一面回答你。"  // 镜中留言
    };
    m_characterDialogues["陈远山"] = QStringList{
        "你藏在一段声音里对吧？那我毁掉所有声音。",
        "{name}，我看你门禁卡刷到了地下三层。那里辐射超标，快上来。",
        "你走到哪里，我就把哪里的音频切断。你是一只泡在静水里的耳朵——没有声音，你就是瞎子。",
        "薇薇，你知道右耳永远安静是什么感觉吗？那不是沉默，那是一个不断提醒你残缺的黑洞。你拒绝军用，等于把我重新推进那个黑洞里。",  // 地下三层隐藏录音
        "开枪吧。反正我这辈子，只听过一半的声音。另一半，是你妈留给我的谎言。"  // 终章自我认知
    };
    m_characterDialogues["老刘"] = QStringList{
        "",  // 老刘不说话，用摩斯电码
        "替我……给她的灵魂带句话。说'门，可以关了。'",  // 终章开口
        "谢谢你还活着。"  // 回声孤儿结局的唇语
    };
    m_characterDialogues["何悦"] = QStringList{
        "周老师，我昨天哼的那首歌你听出来了吗？是《月半小夜曲》哦！",
        "这个实验室好安静，安静得我有点害怕，所以我才一直唱歌……",
        "周老师……救我……他们在门外……",  // 被囚禁时的求救
        "我的声音很干净，对吧？你可以拿去用。这大概是我唯一能帮上忙的地方了。"  // 借出清洁声纹
    };
    m_characterDialogues["底噪"] = QStringList{
        "……你听到了吗？那不是我发出的声音，是你自己在震动。",
        "别害怕低频。它是唯一不会被谎言覆盖的频率。",
        "我已经在这段频率里等了三年。你终于听到了。"
    };
}

void EchoEngine::initEndings()
{
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
        "林薇苏醒，指证陈远山。而你作为'残存{name}'只剩数年寿命，但你们两人在最后时光里合作写了一本《声纹伦理学》——结局文本说：'有些声音不必分清是谁的，只要有人听见，它就没死。'",
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
        "你拼出的画面是：整个A-Lab、陈远山、林薇、甚至{name}，都是某个更高维度'声学模拟程序'中的测试单元。你听到了系统管理员的声音：'第114514次模拟失败，人格分裂度99.8%，建议重启。'然后屏幕出现一行字：'你听到了真相，但你无法被听见。' 游戏强制删除所有存档，回到初始菜单，背景音乐彻底消失。",
        "全周目完成，拼凑出所有红色伪造碎片的集合体"
    };
}

void EchoEngine::initMentorMessages()
{
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

void EchoEngine::initHallucinations()
{
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

void EchoEngine::initNoiseDialogues()
{
    m_noiseDialogues << "……你听到了吗？"
        << "别害怕低频。它是唯一不会被谎言覆盖的频率。"
        << "我已经在这段频率里等了三年。"
        << "你的心跳和我的声纹已经同步。"
        << "陈远山在监听所有高频段，但低频他听不到。"
        << "当你拼出足够多的错误时，我就能说话了。"
        << "每一段错误拼接，都是我的一小片意识在苏醒。"
        << "你越偏离官方版本，就越接近我。";
}

void EchoEngine::initMorseMessages()
{
    m_morseMessages << "跑"        // ... --- ...
        << "别信他"    // 综合
        << "地下"      // 综合
        << "U盘"       // 综合
        << "坐标"      // 综合
        << "她还活着"; // 综合
}

void EchoEngine::initEchoFragments()
{
    m_echoFragments = {
        {"《清洁工的叹息》", "地下二楼洗手间",
         "一位夜班清洁工每天都在凌晨3点录制自己的呼吸声，他说这样'如果哪天我不在了，至少我的呼吸还能陪着这栋楼'。后来他在2038年死于心脏骤停，录音被归档为'设备噪声'。拼出他的呼吸轨迹，会发现它和楼内通风系统的频率完全同步——他的呼吸至今仍在循环。",
         "解锁'呼吸频率'隐蔽通道，可绕过一道门禁。"},
        {"《未寄出的信》", "林薇旧办公桌夹层",
         "一封从未寄出的信，写给她的母亲：'妈，我造出了一种可以存储人格的声音。但我不敢告诉任何人，我把您的笑声也存进去了。现在每次听到那段笑声，我都分不清是您在笑，还是我在哭。'",
         "解锁林薇母亲的原始笑声碎片，逆向播放后是一句方言'回来吃饭'，可作为最终与林薇人格对话时的情感钥匙。"},
        {"《第7号实验体日记》", "地下三层铁皮柜",
         "一个12岁男孩在被移植失败后写下的日记：'叔叔说我是勇敢的小兵。但我现在睡觉时，耳朵里总有一个女声在数羊。她说她是上一位住在这里的姐姐。我数到第114514只羊时，她也变成了一只羊，然后被我吞下去了。'",
         "发现'数字幻觉'机制——某些频率如果叠加特定的整数倍，会触发隐藏闪现画面。"},
        {"《走廊里的掌声》", "主走廊声纹地图",
         "每年9月17日午夜12点，走廊会出现一段长达3秒的掌声录音。追溯源头，是1989年A-Lab成立典礼上，第一批研究员（包括年轻林薇和陈远山）的集体鼓掌。掌声中藏着一段极高频的DNA序列声纹编码——是林薇留给自己的'生物密钥'。",
         "获得'基因锁'破解权限，第四章可打开林薇肉体的维生舱，无需密码。"},
        {"《鸟与静音室》", "顶楼天台",
         "一只被困在天台通风口的乌鸦，它的叫声被录进系统。拼出它的鸣叫频谱，发现它模仿的不是同类，而是林薇的电话铃声。这只鸟是林薇三年前亲手放的，用来提醒自己'如果有一天陈远山开始监听，我会用这个铃声作为暗号'。而你现在听到的，是她一直没等到的来电。",
         "触发隐藏支线'最后的电话'——你用声纹重构仪伪造林薇的铃声，播放后，陈远山手下的自动监听系统会误以为林薇还在外面，短暂转移注意力。"},
        {"《镜子里的第二声道》", "你的公寓卫生镜",
         "某天深夜，你用重构仪扫描自己家的镜子，发现镜面反射声波时产生了0.01秒的延迟——那不是物理延迟，是另一段独立声纹在同步播放。拼出这段'镜中声'，是林薇在你入住前就预录进去的留言：'欢迎回家。这里所有墙壁里都藏着我为你准备的备用碎片。如果有一天你连自己都不敢信了，就对着镜子说话，我会在镜子的另一面回答你。'",
         "解锁'镜面空间'小关卡，可在任何场景中'翻转'声谱，获得另一套碎片。"}
    };
    m_forgottenEntries = {
        {"无名清洁工", "他的呼吸至今仍在这栋楼里循环。"},
        {"林薇的母亲", "她的笑声被女儿存进了声音里，从未消散。"},
        {"第7号实验体", "一个数羊数到第114514只的男孩。"},
        {"1989年A-Lab第一批研究员", "掌声里有他们年轻的回声。"},
        {"天台上的乌鸦", "它模仿着那通永远没等到的电话铃声。"},
        {"老刘的弟弟", "门卫用一辈子，替他守着那扇不该再打开的门。"}
    };
}

void EchoEngine::initEpilogues()
{
    m_epilogues[EndingType::SilentArchive] = {
        EndingType::SilentArchive,
        "你获'优秀员工'后升任副主任。某天你在审查新入职人员的档案时，看到一张照片——一个年轻人笑起来的样子像极了14岁的你。你下意识把耳朵贴向屏幕，听到那人的心跳声节奏与当年的自己完全一致。你拿起公章，在他的'声纹授权书'上盖了通过。背景里林薇的尖叫渐渐变成笑声。"
    };
    m_epilogues[EndingType::EchoOrphan] = {
        EndingType::EchoOrphan,
        "你聋了，但学会了读唇语。某天你在医院复诊，看到一个老人对着空气说话——那是老刘。他对着你微笑，嘴型是：'谢谢你还活着。'你无法回话，但你把那天的阳光在笔记本上画了一道波形。画完后你意识到：那一横是平的，没有起伏，像极了你现在的世界。"
    };
    m_epilogues[EndingType::Duet] = {
        EndingType::Duet,
        "林薇苏醒后的第三年，你们合著的书出版了。首发会上，一个读者举手问：'两位老师，人格声纹如果被滥用，最可怕的后果是什么？'你和林薇对视一眼，同时开口，说的却是同一句话：'最可怕的不是被盗走，而是你开始怀疑，自己原本的声音是否真的属于自己。'台下静默五秒，然后响起你从未听过的最纯粹、最不需要解析的——人类的掌声。"
    };
    m_epilogues[EndingType::Matricide] = {
        EndingType::Matricide,
        "成为主任后，你把所有剩余的实验体声纹全部格式化。最后一个文件是林薇的'生命维持低频'。你手指放在删除键上整整一夜，最终没有删。但在第二天清晨，系统自动弹出一段日志：'低频能量耗尽，目标对象已无生命体征。'你走到窗前，发现天亮了，但你第一次觉得阳光很吵。"
    };
    m_epilogues[EndingType::Rewinder] = {
        EndingType::Rewinder,
        "你和林薇的意识在云端融合后，你们存在于世界每一个麦克风里。某天一个孩子对着智能音箱喊'给我讲个故事'，你们同时回答：'从前有一个世界，那里的人们用耳朵相爱，用沉默背叛。你想听哪个版本？'孩子说：'两个都要。'你们笑了——那是服务器第一次算出'笑声'的无限循环算法。"
    };
    m_epilogues[EndingType::ZeroDecibel] = {
        EndingType::ZeroDecibel,
        "系统强制删除存档后，你重新打开游戏。初始菜单只有一行字：'系统检测到外部环境声。是否继续？'你选择'是'，游戏立刻打开麦克风，录制2秒外部声音，然后播放一段经过实时变调的、完全属于你自己的'声纹档案'——档案标题为：'第114515次模拟，这一次，你选择被听见。'紧接着游戏立即崩溃退出，不再可启动。这是游戏对你最后的、也是唯一一次真实的'交互回声'。"
    };
}

// ═══════════════════════════════════════════
//  碎片生成
// ═══════════════════════════════════════════

void EchoEngine::initFragmentTypes()
{
    m_fragments.clear();
    m_timeline.clear();
    m_nextId = 0;
    m_isDistorted = false;
    m_silenceTriggered = false;
    m_currentAnchor = AnchorType::None;
    // 精简槽位：序章 4、Ch1 4、Ch2 5、Ch3 5、Ch4/Finale 6
    int slotCount = 4 + static_cast<int>(m_currentChapter) / 2;
    if (m_currentChapter >= GameChapter::Chapter4) slotCount = 6;
    for (int i = 0; i < slotCount; ++i) {
        m_timeline.push_back({ i, -1, false, false, AnchorType::None });
    }
}

CommonFragment EchoEngine::createFragment(FragmentType type, Credibility cred, float duration,
    bool isAnchor, AnchorType anchor)
{
    CommonFragment frag;
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
    case FragmentType::Footstep:      setProps("脚步声", "走廊里渐近的脚步声", 200 + (rand() % 800), 0.3f); break;
    case FragmentType::ElectricBuzz:  setProps("电流声", "设备电流嗡鸣", 50 + (rand() % 150), 0.15f); break;
    case FragmentType::WaterDrop:     setProps("滴水声", "水管深处滴水", 400 + (rand() % 600), 0.2f); break;
    case FragmentType::Heartbeat:     setProps("心跳声", "低沉不规律的心跳", 60 + (rand() % 40), 0.5f); break;
    case FragmentType::VoiceWhisper:  setProps("低语声", "无法辨认的低语", 300 + (rand() % 1200), 0.25f); break;
    case FragmentType::StaticNoise:   setProps("白噪声", "持续背景噪声", 1000 + (rand() % 4000), 0.1f); break;
    case FragmentType::DoorCreak:     setProps("门吱呀声", "沉重铁门推开", 800 + (rand() % 2000), 0.4f); break;
    case FragmentType::Typewriter:    setProps("打字机声", "老式打字机敲击", 500 + (rand() % 1000), 0.35f); break;
    case FragmentType::Breath:        setProps("呼吸声", "缓慢沉重的呼吸", 100 + (rand() % 200), 0.2f); break;
    case FragmentType::LowFrequency:  setProps("低频嗡鸣", "超低频振动", 20 + (rand() % 30), 0.08f); break;
    case FragmentType::MetalExpansion:setProps("金属膨胀", "金属热胀冷缩", 300 + (rand() % 500), 0.15f); break;
    case FragmentType::Geomagnetic:   setProps("地磁波动", "微弱地磁波动", 5 + (rand() % 15), 0.05f); break;
    case FragmentType::ClockTick:     setProps("时钟滴答", "规律滴答声", 1000 + (rand() % 500), 0.3f); break;
    case FragmentType::TireScreech:   setProps("轮胎摩擦", "刺耳的急刹声", 2000 + (rand() % 3000), 0.6f); break;
    case FragmentType::RainAmbient:   setProps("雨声", "持续的雨声", 300 + (rand() % 800), 0.25f); break;
    default: break;
    }
    // 锚点音特殊描述
    if (isAnchor) {
        switch (anchor) {
        case AnchorType::DoorBeep: frag.name = "门禁刷卡声"; frag.description = "实验室门禁的电子蜂鸣"; break;
        case AnchorType::CoffeeMachine: frag.name = "咖啡机启动声"; frag.description = "休息区咖啡机启动"; break;
        case AnchorType::HeartMonitor: frag.name = "心率监护仪"; frag.description = "规律的心率监护仪蜂鸣"; break;
        case AnchorType::ClockChime: frag.name = "钟声"; frag.description = "办公室老式挂钟整点报时"; break;
        case AnchorType::KeyTurn: frag.name = "钥匙转动"; frag.description = "金属钥匙在锁孔中转动"; break;
        default: break;
        }
    }
    // 频谱采样
    int sampleCount = 32;
    frag.spectrumSamples.resize(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        float baseFreq = frag.frequencyHz * (1.0f + i * 0.15f);
        float noise = (rand() % 100 - 50) / 100.0f * 0.2f;
        float credMod = 1.0f;
        if (cred == Credibility::Fake) credMod = 0.3f + (rand() % 40) / 100.0f;
        if (cred == Credibility::Noise) credMod = 0.5f + (rand() % 30) / 100.0f;
        frag.spectrumSamples[i] = frag.amplitude * credMod *
            expf(-(baseFreq - frag.frequencyHz) * (baseFreq - frag.frequencyHz)
                / (2.0f * frag.frequencyHz * frag.frequencyHz * 0.01f)) + noise;
    }
    // 逆向播放秘密
    if (type == FragmentType::TireScreech) {
        frag.revealsSecret = true;
        frag.reverseDescription = "轮胎摩擦声倒放后变成了林薇的哭声：'对不起，我只能让你活在我的声音里。'";
    }
    if (type == FragmentType::Heartbeat) {
        frag.revealsSecret = true;
        frag.reverseDescription = "心跳声倒放后是一段加密坐标——指向城郊废弃精神病院。";
    }
    return frag;
}

void EchoEngine::generateFragments(GameChapter chapter)
{
    m_currentChapter = chapter;
    initFragmentTypes();
    // 重置巧妙机制状态
    m_echoes.clear();
    m_fragmentPlaceCount.clear();
    m_silenceIdleAccum = 0.0f;
    // 静音惩罚跨章节累积（不回零，让"被静音"是一个渐进的宿命），
    // 但真实碎片可以逐步抵消
    switch (chapter) {
    case GameChapter::Prologue: setupChapterPrologue(); break;
    case GameChapter::Chapter1: setupChapter1(); break;
    case GameChapter::Chapter2: setupChapter2(); break;
    case GameChapter::Chapter3: setupChapter3(); break;
    case GameChapter::Chapter4: setupChapter4(); break;
    case GameChapter::Finale:   setupFinale(); break;
    }
    // 打乱碎片
    for (int i = static_cast<int>(m_fragments.size()) - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        std::swap(m_fragments[i], m_fragments[j]);
    }
}

// ═══════════════════════════════════════════
//  各章节碎片配置
// ═══════════════════════════════════════════

void EchoEngine::setupChapterPrologue()
{
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
    // 额外碎片（少量，保持取舍感）
    for (int i = 0; i < 3; ++i) {
        FragmentType t = static_cast<FragmentType>(rand() % static_cast<int>(FragmentType::Count));
        m_fragments.push_back(createFragment(t, Credibility::Noise, 1.0f + (rand() % 20) / 10.0f));
    }
    activateMorse();
}

void EchoEngine::setupChapter1()
{
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
    for (int i = 0; i < 4; ++i) {
        FragmentType t = static_cast<FragmentType>(rand() % static_cast<int>(FragmentType::Count));
        Credibility c = (rand() % 100 < 40) ? Credibility::Suspicious : Credibility::Noise;
        m_fragments.push_back(createFragment(t, c, 1.0f + (rand() % 30) / 10.0f));
    }
    // 第一章结束后解锁逆向播放
    unlockReverse();
}

void EchoEngine::setupChapter2()
{
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
    for (int i = 0; i < 5; ++i) {
        FragmentType t = static_cast<FragmentType>(rand() % static_cast<int>(FragmentType::Count));
        Credibility c = (rand() % 100 < 30) ? Credibility::Real : (rand() % 100 < 50) ? Credibility::Suspicious : Credibility::Noise;
        m_fragments.push_back(createFragment(t, c, 1.0f + (rand() % 40) / 10.0f));
    }
    increaseLowFreq();
}

void EchoEngine::setupChapter3()
{
    m_currentAnchor = AnchorType::KeyTurn;
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::KeyTurn;
    }
    // 自我声纹拼接——碎片是两个版本的人生
    m_fragments.push_back(createFragment(FragmentType::DoorCreak, Credibility::Real, 2.0f, true, AnchorType::KeyTurn));
    m_fragments.push_back(createFragment(FragmentType::RainAmbient, Credibility::Real, 3.5f));    // 玩家的童年雨声
    m_fragments.push_back(createFragment(FragmentType::Typewriter, Credibility::Real, 2.0f));     // 林薇决定做声纹研究
    m_fragments.push_back(createFragment(FragmentType::Heartbeat, Credibility::Real, 3.0f));       // 重叠心跳
    m_fragments.push_back(createFragment(FragmentType::TireScreech, Credibility::Suspicious, 2.0f));// 车祸
    m_fragments.push_back(createFragment(FragmentType::VoiceWhisper, Credibility::Suspicious, 4.0f));
    m_fragments.push_back(createFragment(FragmentType::Breath, Credibility::Suspicious, 2.5f));
    m_fragments.push_back(createFragment(FragmentType::ElectricBuzz, Credibility::Noise, 1.5f));
    m_fragments.push_back(createFragment(FragmentType::StaticNoise, Credibility::Noise, 2.0f));
    m_fragments.push_back(createFragment(FragmentType::LowFrequency, Credibility::Fake, 3.5f));
    m_fragments.push_back(createFragment(FragmentType::ClockTick, Credibility::Fake, 1.0f));
    for (int i = 0; i < 6; ++i) {
        FragmentType t = static_cast<FragmentType>(rand() % static_cast<int>(FragmentType::Count));
        Credibility c = (rand() % 100 < 20) ? Credibility::Real : (rand() % 100 < 40) ? Credibility::Suspicious : Credibility::Fake;
        m_fragments.push_back(createFragment(t, c, 1.0f + (rand() % 40) / 10.0f));
    }
    increaseLowFreq();
    increaseLowFreq();
}

void EchoEngine::setupChapter4()
{
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
    for (int i = 0; i < 7; ++i) {
        FragmentType t = static_cast<FragmentType>(rand() % static_cast<int>(FragmentType::Count));
        Credibility c = static_cast<Credibility>(rand() % 4);
        m_fragments.push_back(createFragment(t, c, 1.0f + (rand() % 40) / 10.0f));
    }
    increaseLowFreq();
    increaseLowFreq();
}

void EchoEngine::setupFinale()
{
    m_currentAnchor = AnchorType::ClockChime;
    if (!m_timeline.empty()) {
        m_timeline[0].isAnchorSlot = true;
        m_timeline[0].requiredAnchor = AnchorType::ClockChime;
    }
    for (int i = 0; i < 8; ++i) {
        FragmentType t = static_cast<FragmentType>(rand() % static_cast<int>(FragmentType::Count));
        Credibility c = static_cast<Credibility>(rand() % 4);
        m_fragments.push_back(createFragment(t, c, 1.0f + (rand() % 40) / 10.0f));
    }
    increaseLowFreq(); increaseLowFreq(); increaseLowFreq();
}

// ═══════════════════════════════════════════
//  时间轴操作
// ═══════════════════════════════════════════

bool EchoEngine::placeFragment(int fragmentId, int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_timeline.size())) return false;
    CommonFragment* frag = nullptr;
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

    // ── 巧妙机制 1：陈远山的静音惩罚 ──
    if (frag->credibility == Credibility::Fake) {
        m_silenceTendency += 0.12f;
        if (m_silenceTendency >= 1.0f && !m_rightEarSilenced) {
            m_rightEarSilenced = true;
        }
    } else if (frag->credibility == Credibility::Real) {
        m_silenceTendency -= 0.05f;
        if (m_silenceTendency < 0.0f) m_silenceTendency = 0.0f;
    }

    // ── 巧妙机制 3：碎片回声（放置即产生衰减回声）──
    m_fragmentPlaceCount[fragmentId]++;
    int placeTimes = m_fragmentPlaceCount[fragmentId];
    FragmentEcho echo;
    echo.sourceFragmentId = fragmentId;
    echo.maxTime = 4.0f + placeTimes * 1.5f;  // 共振：重复放置延长回声
    echo.remainingTime = echo.maxTime;
    echo.intensity = 0.3f + placeTimes * 0.15f;  // 共振增强
    if (echo.intensity > 1.0f) echo.intensity = 1.0f;
    m_echoes.push_back(echo);
    // 共振增强偏执（同一个声音反复出现，你会怀疑自己）
    if (placeTimes >= 3) {
        adjustParanoia(0.08f);
    }

    emit fragmentPlaced(fragmentId, slotIndex);
    if (isTimelineFull()) {
        // 章节完成时解锁对应回声碎片（支线叙事）
        unlockEchoFragmentsForChapter(m_currentChapter);
        emit sceneCompleted(static_cast<int>(m_currentChapter), m_isDistorted);
    }
    return true;
}

bool EchoEngine::removeFragment(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_timeline.size())) return false;
    int fragId = m_timeline[slotIndex].fragmentId;
    if (fragId < 0) return false;
    m_timeline[slotIndex].fragmentId = -1;
    for (auto& f : m_fragments) {
        if (f.id == fragId) { f.isPlaced = false; f.timelineSlot = -1; break; }
    }
    m_isDistorted = false;
    // 共振计数递减，避免移除后累积失真（放/移循环导致 placeTimes 虚高）
    auto it = m_fragmentPlaceCount.find(fragId);
    if (it != m_fragmentPlaceCount.end() && it.value() > 0) {
        it.value()--;
    }
    adjustAnxiety(-0.01f);
    emit fragmentRemoved(fragId, slotIndex);
    return true;
}

float EchoEngine::calculateCoherence()
{
    if (placedFragmentCount() < 2) return 1.0f;
    float totalScore = 0.0f; int pairCount = 0;
    Credibility prevCred = Credibility::Real; bool first = true;
    for (const auto& slot : m_timeline) {
        if (slot.fragmentId < 0) continue;
        for (const auto& f : m_fragments) {
            if (f.id == slot.fragmentId) {
                if (!first) {
                    if (f.credibility == prevCred) totalScore += 1.0f;
                    else if (abs(static_cast<int>(f.credibility) - static_cast<int>(prevCred)) == 1) totalScore += 0.5f;
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

void EchoEngine::adjustParanoia(float delta)
{
    m_paranoiaLevel += delta;
    if (m_paranoiaLevel < 0.0f) m_paranoiaLevel = 0.0f;
    if (m_paranoiaLevel > 1.0f) m_paranoiaLevel = 1.0f;
    emit paranoiaChanged(m_paranoiaLevel);
    if (m_paranoiaLevel > 0.7f && (rand() % 100) < 30) {
        emit hallucinationTriggered(m_hallucinations[rand() % m_hallucinations.size()]);
    }
}

void EchoEngine::adjustAnxiety(float delta)
{
    m_anxietyLevel += delta;
    if (m_anxietyLevel < 0.0f) m_anxietyLevel = 0.0f;
    if (m_anxietyLevel > 1.0f) m_anxietyLevel = 1.0f;
    emit anxietyChanged(m_anxietyLevel);
}

int EchoEngine::placedFragmentCount() const
{
    int c = 0; for (auto& s : m_timeline) if (s.fragmentId >= 0) c++; return c;
}

bool EchoEngine::isTimelineFull() const
{
    for (auto& s : m_timeline) { if (s.fragmentId < 0) return false; }
    return true;
}

void EchoEngine::resetCurrentChapter()
{
    generateFragments(m_currentChapter);
    m_paranoiaLevel = 0.3f; m_anxietyLevel = 0.1f;
}

// ═══════════════════════════════════════════
//  锚点音
// ═══════════════════════════════════════════

bool EchoEngine::isAnchorCorrect() const
{
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

void EchoEngine::unlockReverse()
{
    if (!m_reverseUnlocked) {
        m_reverseUnlocked = true;
        emit reverseUnlocked();
    }
}

void EchoEngine::reverseFragment(int fragmentId)
{
    for (auto& f : m_fragments) {
        if (f.id == fragmentId) { f.isReversed = true; break; }
    }
}

bool EchoEngine::isFragmentReversed(int fragmentId) const
{
    for (auto& f : m_fragments) if (f.id == fragmentId) return f.isReversed;
    return false;
}

QString EchoEngine::getReverseDescription(int fragmentId) const
{
    for (auto& f : m_fragments)
        if (f.id == fragmentId && f.revealsSecret) return f.reverseDescription;
    return "";
}

// ═══════════════════════════════════════════
//  导师/幻觉/底噪/摩斯
// ═══════════════════════════════════════════

QString EchoEngine::applyPlayerName(const QString& text) const
{
    QString out = text;
    out.replace(PLAYER_NAME_PLACEHOLDER, playerDisplayName());
    return out;
}

QString EchoEngine::getMentorMessage(int index) const
{
    if (index < 0 || index >= m_mentorMessages.size()) return "";
    return applyPlayerName(m_mentorMessages[index]);
}

QString EchoEngine::getHallucinationText() const
{
    if (m_paranoiaLevel < 0.5f) return "";
    return m_hallucinations[rand() % m_hallucinations.size()];
}

QString EchoEngine::getLowFreqWarning() const
{
    // 低频警告基于底噪强度（lowFreqIntensity），而非偏执值
    if (m_lowFreqIntensity > 0.6f) return "警告：检测到持续低频信号，来源不明。";
    return "";
}

void EchoEngine::increaseLowFreq()
{
    m_lowFreqIntensity += 0.03f;
    if (m_lowFreqIntensity > 1.0f) m_lowFreqIntensity = 1.0f;
    if (m_lowFreqIntensity > 0.5f && (rand() % 100) < 15) {
        emit noiseCommunion(m_noiseDialogues[rand() % m_noiseDialogues.size()]);
    }
    // 极高底噪触发静默模式
    if (m_lowFreqIntensity > 0.85f && !m_silenceTriggered) {
        m_silenceTriggered = true;
        emit silenceMode(true);
    }
}

QString EchoEngine::noiseDialogue() const
{
    if (m_lowFreqIntensity < 0.5f) return "";
    return m_noiseDialogues[rand() % m_noiseDialogues.size()];
}

QString EchoEngine::getMorseMessage() const
{
    if (!m_morseActive) return "";
    // 固定按当前章节选择摩斯消息，避免每帧随机闪烁导致无法阅读
    int idx = static_cast<int>(m_currentChapter) % m_morseMessages.size();
    return m_morseMessages[idx];
}

QString EchoEngine::getCharacterDialogue(const QString& name, int lineIndex) const
{
    auto it = m_characterDialogues.find(name);
    if (it == m_characterDialogues.constEnd()) return "";
    const auto& lines = it.value();
    if (lineIndex < 0 || lineIndex >= lines.size()) return "";
    return applyPlayerName(lines[lineIndex]);
}

void EchoEngine::revealCharacter(int index)
{
    if (index >= 0 && index < static_cast<int>(m_characters.size()))
        m_characters[index].isRevealed = true;
}

// ═══════════════════════════════════════════
//  选择系统
// ═══════════════════════════════════════════

void EchoEngine::recordChoice(PlayerChoice choice)
{
    m_choices.push_back({ choice, m_currentChapter, "" });
    if (choice == PlayerChoice::RejectFusion) setHardMode(true);
}

bool EchoEngine::hasChosen(PlayerChoice choice) const
{
    for (auto& c : m_choices) if (c.choice == choice) return true;
    return false;
}

PlayerChoice EchoEngine::getChapterChoice(GameChapter chapter) const
{
    for (auto& c : m_choices) if (c.chapter == chapter) return c.choice;
    return PlayerChoice::None;
}

// ═══════════════════════════════════════════
//  结局系统
// ═══════════════════════════════════════════

EndingType EchoEngine::calculateEnding() const
{
    bool trustDirector = hasChosen(PlayerChoice::IgnoreSigh);
    auto ch3 = getChapterChoice(GameChapter::Chapter3);
    auto ch4 = getChapterChoice(GameChapter::Chapter4);
    int fakeCount = 0;
    for (auto& f : m_fragments) if (f.credibility == Credibility::Fake && f.isPlaced) fakeCount++;
    int reversedCount = 0;
    for (auto& f : m_fragments) if (f.isReversed) reversedCount++;

    // ═══ 隐藏/真结局（独立于选择，由行为触发）═══
    // ZeroDecibel 真结局：大量伪造碎片 + 高底噪 + 高偏执（玩家"相信伪造声音"的终极代价）
    int placedCount = placedFragmentCount();
    float fakeRatio = placedCount > 0 ? float(fakeCount) / float(placedCount) : 0.0f;
    if (fakeCount >= 5 && fakeRatio >= 0.6f && m_lowFreqIntensity > 0.75f && m_paranoiaLevel > 0.8f)
        return EndingType::ZeroDecibel;
    // Rewinder 倒带者：大量逆向播放（玩家"怀疑一切声音"的终极选择）
    if (reversedCount >= 5) return EndingType::Rewinder;

    // ═══ 主结局：第三章×第四章选择组合，每个组合导向不同结局 ═══
    // 第三章（3 选）：AcceptFusion 接受融合 / RejectFusion 拒绝融合 / SeparateVoices 分离
    // 第四章（2 选）：TransferBack 转录回林薇 / DeleteLifeSupport 删除生命维持

    // 接受融合（愿做林薇的容器）
    if (ch3 == PlayerChoice::AcceptFusion) {
        if (ch4 == PlayerChoice::TransferBack)
            return EndingType::Duet;           // 接受融合 + 转回 → 双声部（最温情）
        return EndingType::EchoOrphan;         // 接受融合 + 删生命维持 → 林薇彻底消失，你成为唯一回声
    }
    // 拒绝融合（坚持做自己）
    if (ch3 == PlayerChoice::RejectFusion) {
        if (ch4 == PlayerChoice::TransferBack)
            return EndingType::Matricide;      // 拒绝融合 + 转回 → 救回林薇却永远失去自己（弑母式决裂）
        return EndingType::SilentArchive;      // 拒绝融合 + 删生命维持 → 抹除一切，成为静默档案
    }
    // 尝试分离（追求两全）
    if (ch3 == PlayerChoice::SeparateVoices) {
        if (ch4 == PlayerChoice::TransferBack)
            return EndingType::Rewinder;       // 分离 + 转回 → 双意识云端融合（倒带者）
        return EndingType::ZeroDecibel;        // 分离 + 删生命维持 → 彻底否定，真结局
    }

    // ═══ 兜底：未做关键选择时，按信任主任/官方锚点导向 ═══
    if (m_rightEarSilenced && trustDirector) return EndingType::SilentArchive;
    if (trustDirector) return EndingType::SilentArchive;
    return EndingType::Duet;
}

EndingInfo EchoEngine::getEndingInfo(EndingType type) const
{
    EndingInfo ei = m_endings.value(type, { EndingType::None,"未知","","",false });
    // 结局文案中的角色名占位符动态替换为玩家输入的名字
    ei.title = applyPlayerName(ei.title);
    ei.description = applyPlayerName(ei.description);
    ei.condition = applyPlayerName(ei.condition);
    return ei;
}

int EchoEngine::unlockedEndingCount() const
{
    int c = 0; for (auto it = m_endings.begin();it != m_endings.end();++it) if (it->unlocked) c++; return c;
}

QString EchoEngine::endingTitle() const { return getEndingInfo(calculateEnding()).title; }
QString EchoEngine::endingDescription() const { return getEndingInfo(calculateEnding()).description; }
bool EchoEngine::isEndingUnlocked(EndingType type) const { return m_endings.value(type, {}).unlocked; }

void EchoEngine::unlockEnding(EndingType type)
{
    auto it = m_endings.find(type);
    if (it != m_endings.end()) it->unlocked = true;
}

// ═══════════════════════════════════════════
//  章节信息
// ═══════════════════════════════════════════

QString EchoEngine::chapterName() const
{
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

QString EchoEngine::chapterDescription() const
{
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

QColor EchoEngine::credibilityToSpectrumColor(Credibility c)
{
    switch (c) {
    case Credibility::Real: return QColor(0, 180, 255);
    case Credibility::Suspicious: return QColor(0, 220, 200);
    case Credibility::Noise: return QColor(255, 200, 0);
    case Credibility::Fake: return QColor(255, 40, 40);
    }
    return Qt::white;
}

// ═══════════════════════════════════════════
//  共鸣度系统（耳蜗共鸣）
// ═══════════════════════════════════════════

void EchoEngine::addResonance(float delta)
{
    m_resonanceLevel += delta;
    if (m_resonanceLevel < 0.0f) m_resonanceLevel = 0.0f;
    if (m_resonanceLevel > 1.0f) m_resonanceLevel = 1.0f;
    // 共鸣度满时自动解锁隐藏关「第一声」（接入 unlockFirstSound）
    if (m_resonanceLevel >= 1.0f && !m_firstSoundUnlocked) {
        m_firstSoundUnlocked = true;
    }
}

// ═══════════════════════════════════════════
//  回声碎片（支线）
// ═══════════════════════════════════════════

void EchoEngine::unlockEchoFragment(int index)
{
    if (index >= 0 && index < static_cast<int>(m_echoFragments.size())) {
        m_echoFragments[index].unlocked = true;
    }
}

void EchoEngine::unlockEchoFragmentsForChapter(GameChapter chapter)
{
    // 6 个回声碎片按章节进度解锁：
    // 序章→清洁工叹息+未寄出的信(0,1)，Ch1→第7号实验体(2)，
    // Ch2→走廊掌声(3)，Ch3→鸟与静音室(4)，Ch4→镜中第二声道(5)
    switch (chapter) {
    case GameChapter::Prologue:
        unlockEchoFragment(0); unlockEchoFragment(1); break;
    case GameChapter::Chapter1:
        unlockEchoFragment(2); break;
    case GameChapter::Chapter2:
        unlockEchoFragment(3); break;
    case GameChapter::Chapter3:
        unlockEchoFragment(4); break;
    case GameChapter::Chapter4:
    case GameChapter::Finale:
        unlockEchoFragment(5); break;
    }
}

bool EchoEngine::isEchoFragmentUnlocked(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_echoFragments.size())) return false;
    return m_echoFragments[index].unlocked;
}

int EchoEngine::unlockedEchoFragmentCount() const
{
    int c = 0;
    for (const auto& f : m_echoFragments) if (f.unlocked) c++;
    return c;
}

QString EchoEngine::forgottenListText() const
{
    QString text = "——被遗忘者名单——\n";
    int listed = 0;
    for (size_t i = 0; i < m_forgottenEntries.size() && i < m_echoFragments.size(); ++i) {
        if (m_echoFragments[i].unlocked) {
            text += m_forgottenEntries[i].name + "：" + m_forgottenEntries[i].epitaph + "\n";
            listed++;
        }
    }
    if (listed == 0) text += "（尚无被记住的人。继续探索吧。）";
    return text;
}

// ═══════════════════════════════════════════
//  结局余波
// ═══════════════════════════════════════════

QString EchoEngine::getEpilogue(EndingType type) const
{
    auto it = m_epilogues.find(type);
    if (it == m_epilogues.end()) return "";
    return it->content;
}

// ═══════════════════════════════════════════
//  动态文案组合系统（减少硬编码）
// ═══════════════════════════════════════════

QString EchoEngine::noiseConsciousnessLine() const
{
    // 底噪意识对玩家的称呼随"静音惩罚"状态变化
    // 右耳失聪越严重，底噪的"语气"越破碎
    QString name = playerDisplayName();
    if (m_rightEarSilenced) {
        return QString("底噪意识断续传来：'%1……别怕。你……听得见我……一半，对吗？你不是被夺舍的容器，你是我的选择。'").arg(name);
    }
    return QString("底噪意识从耳机里传出：'%1，你别怕。你不是被夺舍的容器，你是我的选择。'").arg(name);
}

EchoEngine::ChoiceContent EchoEngine::buildChoice(ChoicePoint point) const
{
    ChoiceContent cc;
    QString name = playerDisplayName();

    switch (point) {
    case ChoicePoint::PrologueSigh: {
        // 序章：老刘的叹息——是否在沉默中回应
        cc.prompt = QString(
            "你拼出来的对话中，林薇对陈远山说：'核心盘我藏好了，你没机会的。'\n"
            "陈远山冷笑：'你藏在一段声音里对吧？那我毁掉所有声音。'\n\n"
            "拼完后，耳机里传来一声不属于这段录音的叹息（老刘在门外敲了三下桌子）。\n"
            "你选择：");
        cc.options = { "忽略叹息，按标准流程提交报告",
                      "记录叹息，今晚加班重听所有录音" };
        break;
    }
    case ChoicePoint::HeYueHostage: {
        cc.prompt = QString(
            "你回到家，白噪音设备里传来何悦的惨叫。你赶回实验室，发现陈远山以'协助调查'名义"
            "把她关进了声纹隔离室——因为何悦无意中录下了陈远山打给军方的加密电话。\n"
            "你选择：");
        cc.options = { "先用重构仪破解门禁救何悦",
                      "先继续追查林薇线索，回头再救" };
        break;
    }
    case ChoicePoint::Identity: {
        // 自我声纹认同：底噪意识 + 何悦状态附加段落动态拼接
        QString prefix;
        if (m_heYueState == HeYueState::Rescued) {
            prefix = QString(
                "你成功破解门禁救出了何悦。她哭着把'清洁声纹'借给你："
                "'我的声音很干净，你可以拿去用。'\n（你获得了绝对参考系音源，后续关卡敌方探测将短暂失效）\n\n");
        } else if (m_heYueState == HeYueState::Brainwashed) {
            prefix = QString(
                "你选择先追查林薇线索。等你回头时，何悦已被灌入'忠诚声纹'，变成陈远山的傀儡。\n"
                "（她将成为你后期必须面对的敌人）\n\n");
        }
        cc.prompt = prefix + noiseConsciousnessLine() + "\n你选择：";
        cc.options = { QString("接受自己是'林薇+%1'的融合体").arg(name),
                      QString("拒绝融合，用'纯粹%1'身份继续（困难模式）").arg(name),
                      "尝试彻底分离两人声音——逆向播放+高频阻断" };
        break;
    }
    case ChoicePoint::LifeTransfer: {
        cc.prompt = QString(
            "林薇的肉身就在隔离室里，极度虚弱。你面前只有一个操作：");
        cc.options = { "将林薇的完整人格声纹从你脑中转录回她体内",
                      "删除林薇肉体的生命维持系统录音，让密钥彻底消失" };
        break;
    }
    default:
        break;
    }
    return cc;
}

// ═══════════════════════════════════════════
//  巧妙机制 1：陈远山的"静音惩罚"
//  （他右耳失聪，你每放一个伪造碎片，就离"被静音"更近一步）
// ═══════════════════════════════════════════

void EchoEngine::resetSilencePunishment()
{
    m_silenceTendency = 0.0f;
    m_rightEarSilenced = false;
}

// ═══════════════════════════════════════════
//  巧妙机制 2：何悦"绝对参考系"（照妖镜）
//  救出何悦后，伪造碎片在她干净声纹的对照下原形毕露
// ═══════════════════════════════════════════

bool EchoEngine::isCredibilityExposed(const CommonFragment& f) const
{
    // 只有救出何悦后才有"绝对参考系"
    if (m_heYueState != HeYueState::Rescued) return false;
    // 伪造碎片被照出：在何悦的干净声纹下，伪造可信度会暴露
    return f.credibility == Credibility::Fake;
}

// ═══════════════════════════════════════════
//  巧妙机制 3：碎片回声（衰减回声，回声迷宫）
// ═══════════════════════════════════════════

void EchoEngine::updateEchoes(float dt)
{
    for (auto it = m_echoes.begin(); it != m_echoes.end();) {
        it->remainingTime -= dt;
        it->intensity *= 0.995f;  // 缓慢衰减
        if (it->remainingTime <= 0.0f || it->intensity < 0.01f) {
            it = m_echoes.erase(it);
        } else {
            ++it;
        }
    }
}

// ═══════════════════════════════════════════
//  巧妙机制 4：老刘的静默摩斯
//  （你越安静，越能听见那个不再说话的人）
// ═══════════════════════════════════════════

void EchoEngine::updateSilenceMorse(float idleSeconds)
{
    if (m_silenceMorseRevealed) return;
    // 只有序章之后、老刘在附近时才触发
    if (m_currentChapter == GameChapter::Prologue) return;
    m_silenceIdleAccum += idleSeconds;
    if (m_silenceIdleAccum >= 8.0f) {  // 静默 8 秒后浮现
        m_silenceMorseRevealed = true;
        m_silenceMorse.revealed = true;
        // 老刘的求救日期倒计时
        m_silenceMorse.message = "……（沉默中，你听见老刘用手指在桌面敲出的摩斯电码）……\n"
            "他弟弟自杀后的第7年，老刘曾在档案库中尝试自杀，被林薇用声波唤醒。\n"
            "他画下的符号，是'那个人还没死'的倒计时。";
    }
}

QString EchoEngine::getSilenceMorseReveal() const
{
    if (!m_silenceMorseRevealed) return "";
    return m_silenceMorse.message;
}

bool EchoEngine::isSilenceMorseReady() const
{
    return m_silenceMorseRevealed;
}
