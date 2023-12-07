#ifndef GR_DIGITIZERS_FAIRPLOT_HPP
#define GR_DIGITIZERS_FAIRPLOT_HPP
#include <implot.h>
#include <implot_internal.h>
#include "event_definitions.hpp"
/*
 * ImPlot Extension to display data with different discrete (or continuous) values as a color coded horizontal bar
 * - for discrete values, show the discrete values either inline or show a legend
 * - for continues values show a color gradient scale
 */
#ifndef IMPLOT_NO_FORCE_INLINE
#ifdef _MSC_VER
#define IMPLOT_INLINE __forceinline
#elif defined(__GNUC__)
#define IMPLOT_INLINE inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
            #define IMPLOT_INLINE inline __attribute__((__always_inline__))
        #else
            #define IMPLOT_INLINE inline
        #endif
    #else
        #define IMPLOT_INLINE inline
#endif
#else
#define IMPLOT_INLINE inline
#endif

namespace FairPlot {
    template<std::size_t N>
    struct ScrollingBuffer {
        std::array<ImPlotPoint, N> data{};
        std::size_t size = 0;
        std::size_t offset = 0;

        bool empty() const { return size == 0; }

        void pushBack(ImPlotPoint newData) {
            data[(offset + size) % N] = newData;
            if (size < N) {
                size++;
            } else {
                offset++;
            }
        }

        void reset() {
            size = 0;
            offset = 0;
        }

        const ImPlotPoint &operator[](std::size_t idx) const {
            return data[(idx + offset) % N];
        }
    };

    template <std::size_t N>
    void PlotStatusBar(const char* label_id, const FairPlot::ScrollingBuffer<N>  &pointBuffer, double xOffset) {
        if (ImPlot::BeginItem(label_id, 0, ImPlotCol_Fill)) {
            ImPlotContext& gp = *ImPlot::GetCurrentContext();
            ImDrawList& draw_list = *ImPlot::GetPlotDrawList();
            const ImPlotNextItemData& s = ImPlot::GetItemData();
            if (pointBuffer.size > 1 && s.RenderFill) {
                const ImPlotPlot& plot   = *gp.CurrentPlot;
                const ImPlotAxis& x_axis = plot.Axes[plot.CurrentX];
                const ImPlotAxis& y_axis = plot.Axes[plot.CurrentY];

                const auto pixY_0 = static_cast<int>(s.LineWeight);
                const float pixY_1_float = s.DigitalBitHeight;
                const auto pixY_1 = static_cast<int>(pixY_1_float); //allow only positive values
                const auto pixY_chPosOffset = static_cast<int>(ImMax(s.DigitalBitHeight, pixY_1_float) + s.DigitalBitGap);
                const int pixY_Offset = 1; //20 pixel from bottom due to mouse cursor label

                int pixYMax = 0;
                ImPlotPoint itemData1 = pointBuffer[0];
                for (std::size_t i = 0; i <= pointBuffer.size; ++i) {
                    ImVec2 pMin;
                    ImVec2 pMax;
                    ImPlotPoint itemData2;
                    if (i < pointBuffer.size) {
                        itemData2 = pointBuffer[i];
                        if (ImNanOrInf(itemData1.y)) {
                            itemData1 = itemData2;
                            continue;
                        }
                        if (ImNanOrInf(itemData2.y)) itemData2.y = ImConstrainNan(ImConstrainInf(itemData2.y));
                        pixYMax = ImMax(pixYMax, pixY_chPosOffset);
                        pMin = ImPlot::PlotToPixels({itemData1.x + xOffset, itemData1.y}, IMPLOT_AUTO, IMPLOT_AUTO);
                        pMax = ImPlot::PlotToPixels({itemData2.x + xOffset, itemData2.y}, IMPLOT_AUTO, IMPLOT_AUTO);
                        pMin.y = (y_axis.PixelMin) + (static_cast<float>((-gp.DigitalPlotOffset) - pixY_Offset));
                        pMax.y = (y_axis.PixelMin) + (static_cast<float>((-gp.DigitalPlotOffset) - pixY_0 - pixY_1 - pixY_Offset));
                        //plot only one rectangle for same digital state
                        while (((i + 2) < pointBuffer.size) && (itemData1.y == itemData2.y)) {
                            const std::size_t in = (i + 1);
                            itemData2 = pointBuffer[in];
                            if (ImNanOrInf(itemData2.y)) break;
                            pMax.x = ImPlot::PlotToPixels({itemData2.x + xOffset, itemData2.y}, IMPLOT_AUTO, IMPLOT_AUTO).x;
                            i++;
                        }
                        //do not extend plot outside plot range
                        if (pMin.x < x_axis.PixelMin) pMin.x = x_axis.PixelMin;
                        if (pMax.x < x_axis.PixelMin) pMax.x = x_axis.PixelMin;
                        if (pMin.x > x_axis.PixelMax) pMin.x = x_axis.PixelMax;
                        if (pMax.x > x_axis.PixelMax) pMax.x = x_axis.PixelMax;
                    } else { // extend last event to the end of the plot
                        pMin = ImPlot::PlotToPixels({itemData1.x + xOffset, itemData1.y}, IMPLOT_AUTO, IMPLOT_AUTO);
                        pMin.y = (y_axis.PixelMin) + (static_cast<float>((-gp.DigitalPlotOffset) - pixY_Offset));
                        pMax = {x_axis.PixelMax, y_axis.PixelMin + (static_cast<float>((-gp.DigitalPlotOffset) - pixY_0 - pixY_1 - pixY_Offset))};
                    }
                    //plot a rectangle that extends up to x2 with y1 height
                    if ((pMax.x > pMin.x) && (gp.CurrentPlot->PlotRect.Contains(pMin) || gp.CurrentPlot->PlotRect.Contains(pMax))) {
                        ImColor color = ImPlot::GetColormapColorU32(static_cast<int>(itemData1.y) % ImPlot::GetColormapSize(), IMPLOT_AUTO);
                        draw_list.AddRectFilled(pMin, pMax, color);
                    }
                    itemData1 = itemData2;
                }
                gp.DigitalPlotItemCnt++;
                gp.DigitalPlotOffset += pixYMax;
            }
            ImPlot::EndItem();
        }
    }

    template <std::size_t N>
    void PlotInfLinesOffset(const char* label_id, const FairPlot::ScrollingBuffer<N>  &pointBuffer, ImPlotInfLinesFlags_ flags, double xOffset) {
        const double yText = ImPlot::GetPlotLimits(IMPLOT_AUTO,IMPLOT_AUTO).Y.Max;
        const ImVec2 textOffset{-8.f, 120.f};
        std::array<double, N> data;
        for (std::size_t i = 0; i < pointBuffer.size; i++) {
            const ImPlotPoint p{pointBuffer[i].x + xOffset, pointBuffer[i].y};
            auto eventno = static_cast<uint16_t>(p.y);
            data[i] = p.x;
            ImPlot::PlotText(fmt::format("{}({})", eventNrTable.contains(eventno) ? eventNrTable.at(eventno).first : "UNKNOWN", eventno).c_str(), p.x, yText, textOffset, ImPlotTextFlags_Vertical);
        }
        ImPlot::PlotInfLines(label_id, data.data(), static_cast<int>(pointBuffer.size), flags, 0, sizeof(double));
    }

    int bpcidColormap() {
        static std::array<ImU32, 13> colorData = [&bpcidColors = bpcidColors]() {
            std::array<ImU32, 13> colors{};
            for (std::size_t i = 0; i < bpcidColors.size(); i++)
                colors[i] = bpcidColors[i];
            return colors;
        }();
        return ImPlot::AddColormap("BPCIDColormap", colorData.data(), colorData.size());
    }
    int bpidColormap() {
        static std::array<ImU32,2> colorData{0xff7dcfb6, 0xff00b2ca};
        return ImPlot::AddColormap("BPIDColormap", colorData.data(), colorData.size());
    }
    int sidColormap() {
        static std::array<ImU32,2> colorData{0xfff79256, 0xfffbd1a2};
        return ImPlot::AddColormap("SIDColormap", colorData.data(), colorData.size());
    }
    int boolColormap() {
        static std::array<ImU32,2> colorData{0x800000ff, 0x8000ff00};
        return ImPlot::AddColormap("BoolColormap", colorData.data(), colorData.size());
    }
}

#endif //GR_DIGITIZERS_FAIRPLOT_HPP
