#include "Gradient.hpp"
#include "Interpolation.hpp"
#include "Settings.hpp"
#include "color_conversions.hpp"
#include "internal.hpp"
#include "color_utils.hpp"

namespace ImGG {

static void draw_uniform_square(
    ImDrawList&  draw_list,
    const ImVec2 top_left_corner,
    const ImVec2 bottom_right_corner,
    const ImU32& color
)
{
    static constexpr auto rounding{1.f};
    draw_list.AddRectFilled(
        top_left_corner,
        bottom_right_corner,
        color,
        rounding
    );
}

static void draw_naive_gradient_between_two_colors(
    ImDrawList&  draw_list,
    ImVec2 const top_left_corner,
    ImVec2 const bottom_right_corner,
    ImU32 const& color_left, ImU32 const& color_right
)
{
    draw_list.AddRectFilledMultiColor(
        top_left_corner,
        bottom_right_corner,
        color_left, color_right, color_right, color_left
    );
}

// We draw two "naive" gradients to reduce the visual artifacts.
// The problem is that ImGui does its color interpolation in sRGB space which doesn't look the best.
// We use Lab for better-looking results.
static void draw_gradient_between_two_colors(
    ImDrawList&   draw_list,
    ImVec2 const  top_left_corner,
    ImVec2 const  bottom_right_corner,
    ImVec4 const& color_left, ImVec4 const& color_right,
    Interpolation interpolationMode
)
{
    glm::vec4 col1 = ColorUtils::srgbToLinear(ColorUtils::convert(color_left));
    glm::vec4 col2 = ColorUtils::srgbToLinear(ColorUtils::convert(color_right));

    const float x1 = top_left_corner.x;
    const float x2 = bottom_right_corner.x;
    const int numSegments = (int)((x2 - x1) / 20.f) + 1;
    glm::vec4 prevCol;
    for (int segmentIdx = 0; segmentIdx < numSegments; ++segmentIdx)
    {
        float t1 = segmentIdx / ((float)numSegments);
        float t2 = (segmentIdx + 1) / ((float)numSegments);
        ImVec2 corner1 = { glm::mix(x1, x2, t1), top_left_corner.y };
        ImVec2 corner2 = { glm::mix(x1, x2, t2), bottom_right_corner.y };

        glm::vec4 segmentCol1;
        if (segmentIdx == 0)
        {
            segmentCol1 = ColorUtils::linearToSrgb(interpolateColors(col1, col2, t1, interpolationMode));
        }
        else
        {
            segmentCol1 = prevCol;
        }
        glm::vec4 segmentCol2 = ColorUtils::linearToSrgb(interpolateColors(col1, col2, t2, interpolationMode));

        draw_naive_gradient_between_two_colors(
            draw_list,
            corner1, corner2,
            ImGui::ColorConvertFloat4ToU32(ColorUtils::convert(segmentCol1)), ImGui::ColorConvertFloat4ToU32(ColorUtils::convert(segmentCol2))
        );

        prevCol = segmentCol2;
    }
}

void draw_gradient(
    ImDrawList&     draw_list,
    const Gradient& gradient,
    const ImVec2    gradient_position,
    const ImVec2    size
)
{
    assert(!gradient.is_empty());
    float current_starting_x = gradient_position.x;
    for (auto mark_iterator = gradient.get_marks().begin(); mark_iterator != gradient.get_marks().end(); ++mark_iterator) // We need to use iterators because we need to access the previous element from time to time
    {
        const Mark& mark = *mark_iterator;

        const auto color_right = mark.color;

        const auto from{current_starting_x};
        const auto to{gradient_position.x + mark.position.get() * (size.x)};
        if (mark.position.get() != 0.f)
        {
            const auto color_left = (mark_iterator != gradient.get_marks().begin())
                                        ? std::prev(mark_iterator)->color
                                        : color_right;

            Interpolation interpolationMode = gradient.interpolation_mode();
            switch (interpolationMode)
            {
            case Interpolation::Constant:
                draw_uniform_square(
                    draw_list,
                    ImVec2{from, gradient_position.y},
                    ImVec2{to, gradient_position.y + size.y},
                    ImGui::ColorConvertFloat4ToU32(color_left)
                );
                break;
            default:
                draw_gradient_between_two_colors(
                    draw_list,
                    ImVec2{from, gradient_position.y},
                    ImVec2{to, gradient_position.y + size.y},
                    color_left, color_right,
                    interpolationMode
                );
                break;
            }
        }
        current_starting_x = to;
    }
    // If the last element is not at the end position, extend its color to the end position
    if (gradient.get_marks().back().position.get() != 1.f)
    {
        draw_uniform_square(
            draw_list,
            ImVec2{current_starting_x, gradient_position.y},
            ImVec2{gradient_position.x + size.x, gradient_position.y + size.y},
            ImGui::ColorConvertFloat4ToU32(gradient.get_marks().back().color)
        );
    }
}

static auto mark_invisible_button(
    const ImVec2 position_to_draw_mark,
    const float  mark_square_size,
    const float  gradient_height
) -> bool
{
    ImGui::SetCursorScreenPos(position_to_draw_mark - ImVec2{mark_square_size * 1.5f, gradient_height});
    const auto button_size = ImVec2{
        mark_square_size * 3.f,
        gradient_height + mark_square_size * 2.f};
    ImGui::InvisibleButton("mark", button_size, ImGuiButtonFlags_MouseButtonMiddle | ImGuiButtonFlags_MouseButtonLeft);
    return ImGui::IsItemHovered();
}

static void draw_mark(
    ImDrawList&  draw_list,
    const ImVec2 position_to_draw_mark,
    const float  mark_square_size,
    ImU32        mark_color
)
{
    const auto mark_top_triangle    = ImVec2{0.f, -mark_square_size};
    const auto mark_bottom_triangle = ImVec2{mark_square_size, 0.f};
    draw_list.AddTriangleFilled(
        position_to_draw_mark + mark_top_triangle,
        position_to_draw_mark - mark_bottom_triangle,
        position_to_draw_mark + mark_bottom_triangle,
        mark_color
    );

    const auto mark_top_left_corner =
        ImVec2{-mark_square_size - 1.f, 0.f};
    const auto mark_bottom_right_corner =
        ImVec2{
            mark_square_size + 1.f,
            2.f * mark_square_size};
    draw_uniform_square(
        draw_list,
        position_to_draw_mark + mark_top_left_corner,
        position_to_draw_mark + mark_bottom_right_corner,
        mark_color
    );

    static constexpr auto offset_between_mark_square_and_mark_square_inside = ImVec2{1.f, 1.f};
    draw_uniform_square(
        draw_list,
        position_to_draw_mark + mark_top_left_corner + offset_between_mark_square_and_mark_square_inside,
        position_to_draw_mark + mark_bottom_right_corner - offset_between_mark_square_and_mark_square_inside,
        mark_color
    );
}

void draw_marks(
    ImDrawList&     draw_list,
    const ImVec2    position_to_draw_mark,
    const ImU32     mark_color,
    const float     gradient_height,
    const bool      mark_is_selected,
    Settings const& settings
)
{
    static constexpr auto mark_square_size{6.f};

    bool const is_hovered = mark_invisible_button(position_to_draw_mark, mark_square_size, gradient_height);

    draw_mark(
        draw_list,
        position_to_draw_mark,
        mark_square_size,
        is_hovered
            ? (mark_is_selected
                   ? settings.mark_selected_hovered_color
                   : settings.mark_hovered_color)
            : (mark_is_selected
                   ? settings.mark_selected_color
                   : settings.mark_color)
    );

    static constexpr auto square_size{3.f};
    static constexpr auto mark_top_left_corner =
        ImVec2{-square_size, square_size};
    static constexpr auto mark_bottom_right_corner =
        ImVec2{square_size, square_size * square_size};
    draw_uniform_square(
        draw_list,
        position_to_draw_mark + mark_top_left_corner,
        position_to_draw_mark + mark_bottom_right_corner,
        mark_color
    );
}

} // namespace ImGG