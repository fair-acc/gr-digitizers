#ifndef GR_DIGITIZERS_PLOT_HPP
#define GR_DIGITIZERS_PLOT_HPP
#include <implot.h>
#include <implot_internal.h>
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


enum ImPlotStatusBarFlags {
    ImPlotStatusBarFlags_None      = 0,      // default
    ImPlotStatusBarFlags_Bool      = 1 << 0, // Boolean Value: alternate between on of
    ImPlotStatusBarFlags_Discrete  = 1 << 1, // set discrete colors
    ImPlotStatusBarFlags_Alternate = 1 << 2, // alternate between to different colors on value change
    ImPlotStatusBarFlags_Continuous = 1 << 3, // get continuous color from scale
};

namespace ImPlot {
    template <typename _IndexerX, typename _IndexerY>
    struct GetterXY {
        GetterXY(_IndexerX x, _IndexerY y, int count) : IndxerX(x), IndxerY(y), Count(count) { }
        template <typename I> IMPLOT_INLINE ImPlotPoint operator()(I idx) const {
            return ImPlotPoint(IndxerX(idx),IndxerY(idx));
        }
        const _IndexerX IndxerX;
        const _IndexerY IndxerY;
        const int Count;
    };

    /// Interprets a user's function pointer as ImPlotPoints
    struct GetterFuncPtr {
        GetterFuncPtr(ImPlotGetter getter, void* const data, int count) :
                Getter(getter),
                Data(data),
                Count(count)
        { }
        template <typename I> IMPLOT_INLINE ImPlotPoint operator()(I idx) const {
            return Getter(idx, Data);
        }
        ImPlotGetter Getter;
        void* const Data;
        const int Count;
    };

    template <typename T>
    struct IndexerIdx {
        IndexerIdx(const T* data, int count, int offset = 0, int stride = sizeof(T)) :
                Data(data),
                Count(count),
                Offset(count ? ImPosMod(offset, count) : 0),
                Stride(stride)
        { }
        template <typename I> IMPLOT_INLINE double operator()(I idx) const {
            return (double)IndexData(Data, idx, Count, Offset, Stride);
        }
        const T* Data;
        int Count;
        int Offset;
        int Stride;
    };
//-----------------------------------------------------------------------------
// [SECTION] PlotStateBars
//-----------------------------------------------------------------------------

// TODO: Make this behave like all the other plot types (.e. not fixed in y axis)

    template <typename Getter>
    void PlotStatusBarEx(const char* label_id, Getter getter, ImPlotStatusBarFlags flags) {
        if (BeginItem(label_id, flags, ImPlotCol_Fill)) {
            ImPlotContext& gp = *ImPlot::GetCurrentContext();
            ImDrawList& draw_list = *GetPlotDrawList();
            const ImPlotNextItemData& s = GetItemData();
            if (getter.Count > 1 && s.RenderFill) {
                ImPlotPlot& plot   = *gp.CurrentPlot;
                ImPlotAxis& x_axis = plot.Axes[plot.CurrentX];
                ImPlotAxis& y_axis = plot.Axes[plot.CurrentY];

                int pixYMax = 0;
                ImPlotPoint itemData1 = getter(0);
                bool alternate = false;
                for (int i = 0; i < getter.Count; ++i) {
                    ImPlotPoint itemData2 = getter(i);
                    if (ImNanOrInf(itemData1.y)) {
                        itemData1 = itemData2;
                        continue;
                    }
                    if (ImNanOrInf(itemData2.y)) itemData2.y = ImConstrainNan(ImConstrainInf(itemData2.y));
                    int pixY_0 = (int)(s.LineWeight);
                    float pixY_1_float = s.DigitalBitHeight;
                    int pixY_1 = (int)(pixY_1_float); //allow only positive values
                    int pixY_chPosOffset = (int)(ImMax(s.DigitalBitHeight, pixY_1_float) + s.DigitalBitGap);
                    pixYMax = ImMax(pixYMax, pixY_chPosOffset);
                    ImVec2 pMin = PlotToPixels(itemData1,IMPLOT_AUTO,IMPLOT_AUTO);
                    ImVec2 pMax = PlotToPixels(itemData2,IMPLOT_AUTO,IMPLOT_AUTO);
                    int pixY_Offset = 0; //20 pixel from bottom due to mouse cursor label
                    pMin.y = (y_axis.PixelMin) + ((-gp.DigitalPlotOffset)                   - pixY_Offset);
                    pMax.y = (y_axis.PixelMin) + ((-gp.DigitalPlotOffset) - pixY_0 - pixY_1 - pixY_Offset);
                    //plot only one rectangle for same digital state
                    while (((i+2) < getter.Count) && (itemData1.y == itemData2.y)) {
                        const int in = (i + 1);
                        itemData2 = getter(in);
                        if (ImNanOrInf(itemData2.y)) break;
                        pMax.x = PlotToPixels(itemData2,IMPLOT_AUTO,IMPLOT_AUTO).x;
                        i++;
                    }
                    //do not extend plot outside plot range
                    if (pMin.x < x_axis.PixelMin) pMin.x = x_axis.PixelMin;
                    if (pMax.x < x_axis.PixelMin) pMax.x = x_axis.PixelMin;
                    if (pMin.x > x_axis.PixelMax) pMin.x = x_axis.PixelMax;
                    if (pMax.x > x_axis.PixelMax) pMax.x = x_axis.PixelMax;
                    //plot a rectangle that extends up to x2 with y1 height
                    if ((pMax.x > pMin.x) && (gp.CurrentPlot->PlotRect.Contains(pMin) || gp.CurrentPlot->PlotRect.Contains(pMax))) {
                        ImColor color;
                        if (flags & ImPlotStatusBarFlags_Bool) {
                            color = ImGui::GetColorU32(s.Colors[itemData1.y > 0.0]);
                        } else if (flags & ImPlotStatusBarFlags_Discrete) {
                            color = ImPlot::GetColormapColorU32(static_cast<int>(itemData1.y) % ImPlot::GetColormapSize(), IMPLOT_AUTO);
                        } else if (flags & ImPlotStatusBarFlags_Alternate) {
                            color = ImGui::GetColorU32(s.Colors[alternate = !alternate]);
                        } else if (flags & ImPlotStatusBarFlags_Continuous) {
                            color = ImPlot::SampleColormap(static_cast<float>(itemData1.y));
                        }
                        draw_list.AddRectFilled(pMin, pMax, color);
                    }
                    itemData1 = itemData2;
                }
                gp.DigitalPlotItemCnt++;
                gp.DigitalPlotOffset += pixYMax;
            }
            EndItem();
        }
    }

    template <typename T>
    void PlotStatusBar(const char* label_id, const T* xs, const T* ys, int count, ImPlotStatusBarFlags flags, int offset, int stride) {
        GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
        return PlotStatusBarEx(label_id, getter, flags);
    }
//#define INSTANTIATE_MACRO(T) template IMPLOT_API void PlotStatusBar<T>(const char* label_id, const T* xs, const T* ys, int count, ImPlotStatusBarFlags flags, int offset, int stride);
//    CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
//#undef INSTANTIATE_MACRO

// custom
    void PlotStatusBarG(const char* label_id, ImPlotGetter getter_func, void* const data, int count, ImPlotStatusBarFlags flags) {
        GetterFuncPtr getter(getter_func,data,count);
        return PlotStatusBarEx(label_id, getter, flags);
    }


}

#endif //GR_DIGITIZERS_PLOT_HPP
