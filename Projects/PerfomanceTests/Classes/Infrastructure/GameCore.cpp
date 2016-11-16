#include "GameCore.h"

#include "Engine/Engine.h"
#include "Platform/DateTime.h"
#include "CommandLine/CommandLineParser.h"
#include "Utils/Utils.h"
#include "Engine/Window.h"
#include "Debug/DVAssertDefaultHandlers.h"

#include "Tests/UniversalTest.h"
#include "Tests/MaterialsTest.h"
#include "Tests/LoadingTest.h"

#include <fstream>
#include <algorithm>

using namespace DAVA;

GameCore::GameCore(DAVA::Engine& e)
    : engine(e)
{
    DVASSERT(instance == nullptr);
    instance = this;

    engine.gameLoopStarted.Connect(this, &GameCore::OnAppStarted);
    engine.gameLoopStopped.Connect(this, &GameCore::OnAppFinished);
    engine.suspended.Connect(this, &GameCore::OnSuspend);
    engine.resumed.Connect(this, &GameCore::OnResume);
    engine.beginFrame.Connect(this, &GameCore::BeginFrame);
    engine.endFrame.Connect(this, &GameCore::EndFrame);
    engine.cleanup.Connect(this, &GameCore::Cleanup);
}

void GameCore::OnAppStarted()
{
    new GraphicsDetect();
    GraphicsDetect::Instance()->ReloadSettings();
    SoundSystem::Instance()->InitFromQualitySettings();

    defaultTestParams.startTime = 0;
    defaultTestParams.endTime = 120000;
    defaultTestParams.targetTime = 120000;

    RegisterTests();
    InitScreenController();

    if (testChain.empty())
    {
        Quit();
    }
}

void GameCore::OnAppFinished()
{
    testFlowController->Finish();
    GraphicsDetect::Instance()->Release();

    for (auto* test : testChain)
    {
        SafeRelease(test);
    }

    Logger::RemoveCustomOutput(&teamCityOutput);
}

void GameCore::OnSuspend()
{
}

void GameCore::OnResume()
{
}

void GameCore::BeginFrame()
{
    testFlowController->BeginFrame();
}

void GameCore::EndFrame()
{
    testFlowController->EndFrame();
}

void GameCore::RegisterTests()
{
    // material test
    Vector<std::pair<String, String>> scenes;
    LoadMaps(MaterialsTest::TEST_NAME, scenes);

    for (const auto& scene : scenes)
    {
        BaseTest::TestParams params = defaultTestParams;
        params.sceneName = scene.first;
        params.scenePath = scene.second;

        testChain.push_back(new MaterialsTest(params));
    }

    // universal test
    scenes.clear();
    LoadMaps(UniversalTest::TEST_NAME, scenes);

    for (const auto& scene : scenes)
    {
        BaseTest::TestParams params = defaultTestParams;
        params.sceneName = scene.first;
        params.scenePath = scene.second;

        testChain.push_back(new UniversalTest(params));
    }

    // loading test
    scenes.clear();
    LoadMaps(LoadingTest::TEST_NAME, scenes);

    for (const auto& scene : scenes)
    {
        BaseTest::TestParams params = defaultTestParams;
        params.sceneName = scene.first;
        params.scenePath = scene.second;

        testChain.push_back(new LoadingTest(params));
    }
}

void GameCore::LoadMaps(const String& testName, Vector<std::pair<String, String>>& mapsVector)
{
    YamlParser* testsParser = YamlParser::Create("~res:/tests.yaml");
    DVASSERT(testsParser, "can't open ~res:/tests.yaml");

    YamlParser* mapsParser = YamlParser::Create("~res:/maps.yaml");
    DVASSERT(mapsParser, "can't open ~res:/maps.yaml");

    YamlNode* testsRootNode = testsParser->GetRootNode();
    YamlNode* mapsRootNode = mapsParser->GetRootNode();

    if (testsRootNode->Get(testName))
    {
        const auto& maps = testsRootNode->Get(testName)->AsVector();

        for (auto mapNameNode : maps)
        {
            const String& mapName = mapNameNode->AsString();
            const String& mapPath = mapsRootNode->Get(mapName)->AsString();

            mapsVector.push_back(std::pair<String, String>(mapName, mapPath));
        }
    }

    SafeRelease(mapsParser);
    SafeRelease(testsParser);
}

void GameCore::OnWindowCreated(DAVA::Window* w)
{
    w->SetSize({ 1024, 768 });
    w->SetTitle("Performance Tests");

    // TODO FullScreen
    //w->SetFullScreen(false);
}

void GameCore::OnWindowResized(DAVA::Window* window, DAVA::float32 w, DAVA::float32 h, DAVA::float32 scaleX, DAVA::float32 scaleY)
{
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
    KeyedArchive* archive = engine.GetOptions();
    uint32 width = window->GetWidth();
    uint32 height = window->GetHeight();

    EngineContext* context = engine.GetContext();
    context->virtualCoordSystem->SetVirtualScreenSize(width, height);
    context->virtualCoordSystem->SetProportionsIsFixed(false);
    context->virtualCoordSystem->RegisterAvailableResourceSize(width, height, "Gfx");
    context->virtualCoordSystem->RegisterAvailableResourceSize(width * 2, height * 2, "Gfx2");
#endif
}

void GameCore::Cleanup()
{
    testChain.clear();
    testFlowController.reset();
    instance = nullptr;
}

String GameCore::GetDeviceName()
{
    String device = "device_";

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
    device += DeviceInfo::GetManufacturer() + DeviceInfo::GetModel();
#else
    device += UTF8Utils::EncodeToUTF8(DeviceInfo::GetName());
#endif

    std::replace(device.begin(), device.end(), ' ', '_');
    std::replace(device.begin(), device.end(), '.', '_');

    return device;
}

void GameCore::InitScreenController()
{
    Random::Instance()->Seed(0);

    Logger::Instance()->AddCustomOutput(&teamCityOutput);
    Logger::Info(GetDeviceName().c_str());

    bool chooserFound = CommandLineParser::Instance()->CommandIsFound("-chooser");
    bool testFound = CommandLineParser::Instance()->CommandIsFound("-test");
    bool withoutUIFound = CommandLineParser::Instance()->CommandIsFound("-without-ui");

    String testForRun;

    if (testFound)
    {
        testForRun = CommandLineParser::Instance()->GetCommandParamAdditional("-test", 0);
    }

    if (chooserFound)
    {
        testFlowController = std::unique_ptr<SingleTestFlowController>(new SingleTestFlowController("", defaultTestParams, !withoutUIFound));
    }
    else if (!testForRun.empty())
    {
        Logger::Instance()->Info(DAVA::Format("Test %s", testForRun.c_str()).c_str());

        BaseTest::TestParams singleTestParams = defaultTestParams;
        ReadSingleTestParams(singleTestParams);

        testFlowController = std::unique_ptr<SingleTestFlowController>(new SingleTestFlowController(testForRun, singleTestParams, !withoutUIFound));
    }
    else
    {
        testFlowController = std::unique_ptr<TestChainFlowController>(new TestChainFlowController(!withoutUIFound));
    }

    testFlowController->Init(testChain);
}

void GameCore::ReadSingleTestParams(BaseTest::TestParams& params)
{
    bool testTimeFound = CommandLineParser::Instance()->CommandIsFound("-test-time");
    bool startTimeFound = CommandLineParser::Instance()->CommandIsFound("-statistic-start-time");
    bool endTimeFound = CommandLineParser::Instance()->CommandIsFound("-statistic-end-time");

    bool testFramesFound = CommandLineParser::Instance()->CommandIsFound("-test-frames");
    bool frameDeltaFound = CommandLineParser::Instance()->CommandIsFound("-frame-delta");

    bool debugFrameFound = CommandLineParser::Instance()->CommandIsFound("-debug-frame");
    bool maxDeltaFound = CommandLineParser::Instance()->CommandIsFound("-max-delta");

    if (testTimeFound)
    {
        String testTimeParam = CommandLineParser::Instance()->GetCommandParamAdditional("-test-time", 0);
        params.targetTime = std::atoi(testTimeParam.c_str());

        if (params.targetTime < 0)
        {
            Logger::Error("Incorrect params. TargetTime < 0");
            GameCore::Instance()->Quit();
        }
    }

    if (startTimeFound)
    {
        if (!endTimeFound)
        {
            Logger::Error("Incorrect params. Set end time for range");
            GameCore::Instance()->Quit();
        }

        String startTime = CommandLineParser::Instance()->GetCommandParamAdditional("-statistic-start-time", 0);
        String endTime = CommandLineParser::Instance()->GetCommandParamAdditional("-statistic-end-time", 0);

        params.startTime = std::atoi(startTime.c_str());
        params.endTime = std::atoi(endTime.c_str());

        int32 timeRange = params.endTime - params.startTime;

        if (timeRange < 100 || params.startTime < 0)
        {
            Logger::Error("Incorrect params. Too small time range");
            GameCore::Instance()->Quit();
        }
    }

    if (testFramesFound)
    {
        String testFramesParam = CommandLineParser::Instance()->GetCommandParamAdditional("-test-frames", 0);
        params.targetFramesCount = std::atoi(testFramesParam.c_str());

        if (params.targetFramesCount < 0)
        {
            Logger::Error("Incorrect params. TargetFramesCount < 0");
            GameCore::Instance()->Quit();
        }
    }

    if (frameDeltaFound)
    {
        String frameDeltaParam = CommandLineParser::Instance()->GetCommandParamAdditional("-frame-delta", 0);
        params.targetFrameDelta = std::atof(frameDeltaParam.c_str());

        if (params.targetFrameDelta < 0.0f)
        {
            Logger::Error("Incorrect params. TargetFrameDelta < 0");
            GameCore::Instance()->Quit();
        }
    }

    if (debugFrameFound)
    {
        String debugFrameParam = CommandLineParser::Instance()->GetCommandParamAdditional("-debug-frame", 0);
        params.frameForDebug = std::atoi(debugFrameParam.c_str());

        if (params.frameForDebug < 0)
        {
            Logger::Error("Incorrect params. DebugFrame < 0");
            GameCore::Instance()->Quit();
        }
    }

    if (maxDeltaFound)
    {
        String maxDeltaParam = CommandLineParser::Instance()->GetCommandParamAdditional("-max-delta", 0);
        params.maxDelta = std::atof(maxDeltaParam.c_str());

        if (params.maxDelta < 0.0f)
        {
            Logger::Error("Incorrect params. MaxDelta < 0");
            GameCore::Instance()->Quit();
        }
    }

    Logger::Instance()->Info(DAVA::Format("Target time : %d", params.targetTime).c_str());
    Logger::Instance()->Info(DAVA::Format("Statistic start time : %d", params.startTime).c_str());
    Logger::Instance()->Info(DAVA::Format("Statistic end time : %d", params.endTime).c_str());
    Logger::Instance()->Info(DAVA::Format("Target frames count : %d", params.targetFramesCount).c_str());
    Logger::Instance()->Info(DAVA::Format("Target frame delta : %f", params.targetFrameDelta).c_str());
    Logger::Instance()->Info(DAVA::Format("Frame for debug : %d", params.frameForDebug).c_str());
    Logger::Instance()->Info(DAVA::Format("Max delta : %f", params.maxDelta).c_str());
}

void GameCore::Quit()
{
    engine.Quit(0);
}

GameCore* GameCore::instance = nullptr;

/////////////////////////////////////////////////////////////////////////////////

KeyedArchive* CreateOptions()
{
    KeyedArchive* appOptions = new KeyedArchive();

    appOptions->SetInt32("rhi_threaded_frame_count", 2);
    appOptions->SetInt32("shader_const_buffer_size", 4 * 1024 * 1024);

    appOptions->SetInt32("max_index_buffer_count", 3 * 1024);
    appOptions->SetInt32("max_vertex_buffer_count", 3 * 1024);
    appOptions->SetInt32("max_const_buffer_count", 16 * 1024);
    appOptions->SetInt32("max_texture_count", 2048);
    appOptions->SetInt32("max_texture_set_count", 2048);
    appOptions->SetInt32("max_sampler_state_count", 128);
    appOptions->SetInt32("max_pipeline_state_count", 1024);
    appOptions->SetInt32("max_depthstencil_state_count", 256);
    appOptions->SetInt32("max_render_pass_count", 64);
    appOptions->SetInt32("max_command_buffer_count", 64);
    appOptions->SetInt32("max_packet_list_count", 64);

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)

    appOptions->SetInt32("orientation", Core::SCREEN_ORIENTATION_LANDSCAPE_AUTOROTATE);
    appOptions->SetInt32("renderer", rhi::ApiIsSupported(rhi::RHI_METAL) ? rhi::RHI_METAL : rhi::RHI_GLES2);
    appOptions->SetBool("iPhone_autodetectScreenScaleFactor", true);
#else
    appOptions->SetInt32("bpp", 32);

#if defined(__DAVAENGINE_WIN_UAP__)
    appOptions->SetInt32("renderer", rhi::RHI_DX11);
#else
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#endif

#endif

    return appOptions;
}

int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
{
    Assert::AddHandler(Assert::DefaultLoggerHandler);

    Vector<String> modules =
    {
      "JobManager",
      "NetCore",
      "LocalizationSystem",
      "SoundSystem",
      "DownloadManager",
    };
    DAVA::Engine e;
    e.Init(eEngineRunMode::GUI_STANDALONE, modules, CreateOptions());

    GameCore core(e);
    return e.Run();
}
