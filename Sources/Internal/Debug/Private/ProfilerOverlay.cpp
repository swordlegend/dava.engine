#include "Debug/ProfilerOverlay.h"
#include "Debug/DVAssert.h"
#include "Debug/DebugColors.h"
#include "Render/Renderer.h"
#include "Render/RHI/dbg_Draw.h"
#include <ostream>

namespace DAVA
{
//==============================================================================

namespace ProfilerOverlay
{
namespace Details
{
static const char* OVERLAY_PASS_MARKER_NAME = "ProfilerOverlay";

//GPU Statistic
//==============================================================================

static const char* GPU_FRAME_MARKER_NAME = "GPUFrame";
static const int32 MARKER_CHART_HEIGHT = 100;
static bool overlayEnabled = false;

static const uint32 HISTORY_SIZE = 600;
static const uint32 NON_FILTERED_COUNT = 10;

using MarkerHistory = List<std::pair<uint64, float32>>;
FastNameMap<MarkerHistory> markerHistory;

uint32 lastGPUFrameIndex = 0;
Vector<Vector<TraceEvent>> currentTraces(3);

//==============================================================================

void UpdateHistory(const Vector<Vector<TraceEvent>>& traces)
{
    for (FastNameMap<MarkerHistory>::HashMapItem& i : markerHistory)
    {
        MarkerHistory& history = i.second;
        history.push_back({ 0, 0.f });
        while (uint32(history.size()) > HISTORY_SIZE)
            history.pop_front();
    }

    for (const Vector<TraceEvent>& trace : traces)
    {
        Vector<TraceEvent>::const_iterator traceEnd = trace.cend();
        for (Vector<TraceEvent>::const_iterator it = trace.cbegin(); it != traceEnd; ++it)
        {
            const TraceEvent& e = (*it);
            if (e.phase != TraceEvent::PHASE_BEGIN && e.phase != TraceEvent::PHASE_DURATION)
                continue;

            MarkerHistory& history = markerHistory[e.name];
            if (history.size() == 0)
                history.push_back({ 0, 0.f });

            if (e.phase == TraceEvent::PHASE_DURATION)
            {
                markerHistory[e.name].back().first += e.duration;
            }
            else if (e.phase)
            {
                Vector<TraceEvent>::const_iterator found = std::find_if(it, traceEnd, [&e](const TraceEvent& e1) {
                    return e.name == e1.name && e1.phase == TraceEvent::PHASE_END;
                });

                DVASSERT(found != traceEnd);

                markerHistory[e.name].back().first += found->timestamp - e.timestamp;
            }
        }
    }

    for (FastNameMap<MarkerHistory>::HashMapItem& i : markerHistory)
    {
        MarkerHistory& history = i.second;

        if (history.size() < NON_FILTERED_COUNT)
        {
            history.back().second = float32(history.back().first);
        }
        else
        {
            history.back().second = (++history.rbegin())->second * 0.99f + history.back().first * 0.01f;
        }
    }
}

void Update()
{
    const GPUProfiler::FrameInfo& frameInfo = GPUProfiler::globalProfiler->GetLastFrame();
    if (Details::lastGPUFrameIndex != frameInfo.frameIndex)
    {
        Details::lastGPUFrameIndex = frameInfo.frameIndex;
        Details::currentTraces[0] = std::move(frameInfo.GetTrace());

        Details::UpdateHistory(Details::currentTraces);
    }
}

void DrawHistory(const MarkerHistory& history, const FastName& name, const Rect2i& rect)
{
    static const uint32 CHARTRECT_COLOR = rhi::NativeColorRGBA(0.f, 0.f, 1.f, .4f);
    static const uint32 CHART_COLOR = rhi::NativeColorRGBA(.5f, .11f, .11f, 1.f);
    static const uint32 CHART_FILTERED_COLOR = rhi::NativeColorRGBA(1.f, .18f, .18f, 1.f);
    static const uint32 TEXT_COLOR = rhi::NativeColorRGBA(1.f, 1.f, 1.f, 1.f);
    static const uint32 LINE_COLOR = rhi::NativeColorRGBA(.5f, 0.f, 0.f, 1.f);

    static const int32 MARGIN = 3;
    static const int32 PADDING = 4;
    static const int32 TEXT_COLUMN_CHARS = 9;
    static const int32 TEXT_COLUMN_WIDTH = DbgDraw::NormalCharW * TEXT_COLUMN_CHARS;
    static const uint64 CHART_CEIL_STEP = 500; //mcs

    Rect2i drawRect(rect);
    drawRect.x += PADDING;
    drawRect.y += PADDING;
    drawRect.dx -= 2 * PADDING;
    drawRect.dy -= 2 * PADDING;

    Rect2i chartRect(drawRect);
    chartRect.x += MARGIN + TEXT_COLUMN_WIDTH + MARGIN;
    chartRect.y += MARGIN + DbgDraw::NormalCharH;
    chartRect.dx -= TEXT_COLUMN_WIDTH + 3 * MARGIN;
    chartRect.dy -= 2 * MARGIN + DbgDraw::NormalCharH;

    uint64 maxValue = 0;
    for (const std::pair<uint64, float32>& h : history)
        maxValue = Max(maxValue, h.first);

    char strbuf[128];
    float32 ceilValue = float32((maxValue / CHART_CEIL_STEP + 1) * CHART_CEIL_STEP);

    DbgDraw::FilledRect2D(drawRect.x, drawRect.y, drawRect.x + drawRect.dx, drawRect.y + drawRect.dy, CHARTRECT_COLOR);

    DbgDraw::Line2D(chartRect.x, chartRect.y, chartRect.x, chartRect.y + chartRect.dy, LINE_COLOR);
    DbgDraw::Line2D(chartRect.x, chartRect.y + chartRect.dy, chartRect.x + chartRect.dx, chartRect.y + chartRect.dy, LINE_COLOR);

    const float32 chartstep = float32(chartRect.dx) / history.size();
    const float32 valuescale = chartRect.dy / ceilValue;

    const int32 chart0x = chartRect.x;
    const int32 chart0y = chartRect.y + chartRect.dy;

#define CHART_VALUE_HEIGHT(value) int32(value* valuescale)

    MarkerHistory::const_iterator it = history.begin();
    uint64 value = it->first;
    float32 filtered = it->second;
    ++it;

    int32 px = 0;
    int32 py = CHART_VALUE_HEIGHT(value);
    int32 pfy = CHART_VALUE_HEIGHT(filtered);

    int32 index = 0;
    MarkerHistory::const_iterator hend = history.end();
    for (; it != hend; ++it)
    {
        ++index;

        value = it->first;
        filtered = it->second;

        int32 x = int32(index * chartstep);
        int32 y = CHART_VALUE_HEIGHT(value);
        int32 fy = CHART_VALUE_HEIGHT(filtered);

        DbgDraw::Line2D(chart0x + px, chart0y - py, chart0x + x, chart0y - y, CHART_COLOR);
        DbgDraw::Line2D(chart0x + px, chart0y - pfy, chart0x + x, chart0y - fy, CHART_FILTERED_COLOR);

        px = x;
        py = y;
        pfy = fy;
    }
#undef CHART_VALUE_HEIGHT

    DbgDraw::Text2D(drawRect.x + MARGIN, drawRect.y + MARGIN, TEXT_COLOR, "\'%s\'", name.c_str());

    const int32 lastvalueIndent = (drawRect.dx - 2 * MARGIN) / DbgDraw::NormalCharW;
    sprintf(strbuf, "%lld [%.1f] mcs", value, filtered);
    DbgDraw::Text2D(drawRect.x + MARGIN, drawRect.y + MARGIN, TEXT_COLOR, "%*s", lastvalueIndent, strbuf);

    sprintf(strbuf, "%d mcs", int32(ceilValue));
    DbgDraw::Text2D(drawRect.x + MARGIN, drawRect.y + MARGIN + DbgDraw::NormalCharH, TEXT_COLOR, "%*s", TEXT_COLUMN_CHARS, strbuf);
    DbgDraw::Text2D(drawRect.x + MARGIN, drawRect.y + drawRect.dy - MARGIN - DbgDraw::NormalCharH, TEXT_COLOR, "%*s", TEXT_COLUMN_CHARS, "0 mcs");
}

void DrawTrace(const Vector<TraceEvent>& trace, const char* traceHead, const Rect2i& rect)
{
    static const int32 MARGIN = 3;
    static const int32 PADDING = 4;
    static const int32 LEGEND_ICON_SIZE = DbgDraw::NormalCharH;
    static const int32 TRACE_LINE_HEIGHT = DbgDraw::NormalCharH;
    static const int32 DURATION_TEXT_WIDTH_CHARS = 13;

    static const uint32 BACKGROUND_COLOR = rhi::NativeColorRGBA(0.f, 0.f, 1.f, .4f);
    static const uint32 TEXT_COLOR = rhi::NativeColorRGBA(1.f, 1.f, 1.f, 1.f);
    static const uint32 LINE_COLOR = rhi::NativeColorRGBA(.5f, 0.f, 0.f, 1.f);

    static FastNameMap<uint32> traceColors;

    Rect2i drawRect(rect);
    drawRect.x += PADDING;
    drawRect.y += PADDING;
    drawRect.dx -= 2 * PADDING;
    drawRect.dy -= 2 * PADDING;

    DbgDraw::FilledRect2D(drawRect.x, drawRect.y, drawRect.x + drawRect.dx, drawRect.y + drawRect.dy, BACKGROUND_COLOR);

    if (!trace.size())
        return;

    uint32 maxNameLen = 0;
    uint64 maxTimestamp = 0;
    uint64 minTimestamp = uint64(-1);
    FastNameMap<uint64> eventsDuration;
    for (const TraceEvent& e : trace)
    {
        minTimestamp = Min(minTimestamp, e.timestamp);
        maxTimestamp = (e.phase == TraceEvent::PHASE_DURATION) ? Max(maxTimestamp, e.timestamp + e.duration) : Max(maxTimestamp, e.timestamp);

        maxNameLen = Max(maxNameLen, strlen(e.name.c_str()));

        if (e.phase == TraceEvent::PHASE_DURATION)
            eventsDuration[e.name] += e.duration;
        else if (e.phase == TraceEvent::PHASE_BEGIN)
            eventsDuration[e.name] -= e.timestamp;
        else if (e.phase == TraceEvent::PHASE_END)
            eventsDuration[e.name] += e.timestamp;

        if (traceColors.count(e.name) == 0)
        {
            static uint32 colorIndex = 0;
            traceColors.Insert(e.name, rhi::NativeColorRGBA(CIEDE2000Colors[colorIndex % CIEDE2000_COLORS_COUNT]));
            ++colorIndex;
        }
    }

    int32 x0, x1, y0, y1;

    //Draw Head
    x0 = drawRect.x + MARGIN;
    y0 = drawRect.y + MARGIN;
    DbgDraw::Text2D(x0, y0, TEXT_COLOR, traceHead);

    //Draw Legend (color rects + event name) and total events duration
    int32 legentWidth = LEGEND_ICON_SIZE + DbgDraw::NormalCharW + maxNameLen * DbgDraw::NormalCharW + DbgDraw::NormalCharW;
    y0 += DbgDraw::NormalCharH + MARGIN;
    x1 = x0 + LEGEND_ICON_SIZE;

    char strbuf[256];
    for (FastNameMap<uint64>::iterator it = eventsDuration.begin(); it != eventsDuration.end(); ++it)
    {
        y1 = y0 + LEGEND_ICON_SIZE;

        DbgDraw::FilledRect2D(x0, y0, x1, y1, traceColors[(*it).first]);
        DbgDraw::Text2D(x1 + DbgDraw::NormalCharW, y0, TEXT_COLOR, (*it).first.c_str());

        sprintf(strbuf, "[%*d mcs]", DURATION_TEXT_WIDTH_CHARS - 6, (*it).second);
        DbgDraw::Text2D(x0 + legentWidth, y0, TEXT_COLOR, strbuf);

        y0 += LEGEND_ICON_SIZE + 1;
    }

    //Draw separator
    int32 durationTextWidth = DURATION_TEXT_WIDTH_CHARS * DbgDraw::NormalCharW;
    x0 = drawRect.x + MARGIN + legentWidth + durationTextWidth + MARGIN;
    x1 = x0;
    y0 = drawRect.y + MARGIN + DbgDraw::NormalCharH + MARGIN;
    y1 = drawRect.y + drawRect.dy - MARGIN;
    DbgDraw::Line2D(x0, y0, x1, y1, LINE_COLOR);

    //Draw trace rects
    int32 x0trace = drawRect.x + MARGIN + legentWidth + durationTextWidth + MARGIN * 2;
    int32 y0trace = drawRect.y + MARGIN + DbgDraw::NormalCharH;
    int32 traceWidth = drawRect.dx - x0trace - MARGIN;
    float32 dt = float32(traceWidth) / (maxTimestamp - minTimestamp);

    Vector<std::pair<uint64, uint64>> timestampsStack;
    for (const TraceEvent& e : trace)
    {
        while (timestampsStack.size() && (timestampsStack.back().second != 0) && (e.timestamp >= timestampsStack.back().second))
            timestampsStack.pop_back();

        if (e.phase == TraceEvent::PHASE_DURATION)
        {
            timestampsStack.emplace_back(std::pair<uint64, uint64>(e.timestamp, e.timestamp + e.duration));
        }
        else if (e.phase == TraceEvent::PHASE_BEGIN)
        {
            timestampsStack.emplace_back(std::pair<uint64, uint64>(e.timestamp, 0));
        }
        else if (e.phase == TraceEvent::PHASE_END)
        {
            timestampsStack.back().second = e.timestamp;
        }

        if (e.phase == TraceEvent::PHASE_END || e.phase == TraceEvent::PHASE_DURATION)
        {
            x0 = x0trace + int32((timestampsStack.back().first - trace.front().timestamp) * dt);
            x1 = x0 + int32((timestampsStack.back().second - timestampsStack.back().first) * dt);
            y0 = y0trace + int32(timestampsStack.size() * TRACE_LINE_HEIGHT);
            y1 = y0 + TRACE_LINE_HEIGHT;
            DbgDraw::FilledRect2D(x0, y0, x1, y1, traceColors[e.name]);
        }
    }
}

void DrawOverlay()
{
    DbgDraw::EnsureInited();
    DbgDraw::SetScreenSize(uint32(Renderer::GetFramebufferWidth()), uint32(Renderer::GetFramebufferHeight()));
    DbgDraw::SetNormalTextSize();

    Rect2i chartRect(0, 0, Renderer::GetFramebufferWidth() / 2, MARKER_CHART_HEIGHT);
    FastNameMap<MarkerHistory>::iterator hend = markerHistory.end();
    for (FastNameMap<MarkerHistory>::iterator it = markerHistory.begin(); it != hend; ++it)
    {
        DrawHistory(it->second, it->first, chartRect);
        chartRect.y += chartRect.dy;
        if ((chartRect.y + chartRect.dy) > 3 * Renderer::GetFramebufferHeight() / 4)
        {
            chartRect.x = Renderer::GetFramebufferWidth() / 2;
            chartRect.y = 0;
        }
    }

    DrawTrace(currentTraces[0], Format("Frame %d", lastGPUFrameIndex).c_str(),
              Rect2i(0, 3 * Renderer::GetFramebufferHeight() / 4, Renderer::GetFramebufferWidth(), Renderer::GetFramebufferHeight() / 4));

    //////////////////////////////////////////////////////////////////////////

    rhi::RenderPassConfig passConfig;
    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_LOAD;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    passConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_NONE;
    passConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    passConfig.priority = PRIORITY_MAIN_2D - 10;
    passConfig.viewport.x = 0;
    passConfig.viewport.y = 0;
    passConfig.viewport.width = Renderer::GetFramebufferWidth();
    passConfig.viewport.height = Renderer::GetFramebufferHeight();
    DAVA_GPU_PROFILER_RENDER_PASS(passConfig, OVERLAY_PASS_MARKER_NAME);

    rhi::HPacketList packetList;
    rhi::HRenderPass pass = rhi::AllocateRenderPass(passConfig, 1, &packetList);
    rhi::BeginRenderPass(pass);
    rhi::BeginPacketList(packetList);

    DbgDraw::FlushBatched(packetList);

    rhi::EndPacketList(packetList);
    rhi::EndRenderPass(pass);
}

}; //ns Details

//==============================================================================
//Public:
void Enable()
{
    Details::overlayEnabled = true;
}

void Disable()
{
    Details::overlayEnabled = false;
}

void OnFrameEnd()
{
    if (!Details::overlayEnabled)
        return;

    Details::Update();
    Details::DrawOverlay();
}
}; //ns ProfilerOverlay
}; //ns