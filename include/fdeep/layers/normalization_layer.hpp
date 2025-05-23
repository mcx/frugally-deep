// Copyright 2016, Tobias Hermann.
// https://github.com/Dobiasd/frugally-deep
// Distributed under the MIT License.
// (See accompanying LICENSE file or at
//  https://opensource.org/licenses/MIT)

#pragma once

#include "fdeep/layers/layer.hpp"

#include <fplus/fplus.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace fdeep {
namespace internal {

    class normalization_layer : public layer {
    public:
        explicit normalization_layer(
            const std::string& name,
            const std::vector<int>& axes,
            const float_vec& mean, const float_vec& variance,
            bool invert)
            : layer(name)
            , axes_(axes)
            , mean_(mean)
            , variance_(variance)
            , invert_(invert)
        {
            assertion(axes.size() <= 1, "Unsupported number of axes for Normalization layer");
        }

    protected:
        tensors apply_impl(const tensors& inputs) const override final
        {
            const auto& input = single_tensor_from_tensors(inputs);

            const int rank = static_cast<int>(input.shape().rank());

            const std::function<tensor(const std::size_t, const tensor&)> transform_slice_default =
                [&](const std::size_t idx, const tensor& slice) -> tensor {
                const auto sqrt_of_variance = std::sqrt(variance_[idx]);
                return transform_tensor([&](float_type x) { return (x - mean_[idx]) / std::fmax(sqrt_of_variance, 1e-7); }, slice);
            };
            const std::function<tensor(const std::size_t, const tensor&)> transform_slice_invert =
                [&](const std::size_t idx, const tensor& slice) -> tensor {
                const auto sqrt_of_variance = std::sqrt(variance_[idx]);
                return transform_tensor([&](float_type x) { return (mean_[idx] + (x * std::fmax(sqrt_of_variance, 1e-7))); }, slice);
            };
            const auto transform_slice = invert_ ? transform_slice_invert : transform_slice_default;

            if (axes_.empty()) {
                assertion(variance_.size() == 1, "Invalid number of variance values in Normalization layer.");
                return { transform_slice(0, input) };
            }

            const auto axis_dim = axes_[0] == -1 ? 0 : rank - axes_[0];

            const auto transform_slice_with_idx = [&](const tensors& slices) -> tensors {
                assertion(variance_.size() == slices.size(), "Invalid number of variance values in Normalization layer.");
                return fplus::transform_with_idx(transform_slice, slices);
            };

            if (axis_dim == 0)
                return { concatenate_tensors_depth(transform_slice_with_idx(tensor_to_depth_slices(input))) };
            else if (axis_dim == 1)
                return { concatenate_tensors_width(transform_slice_with_idx(tensor_to_tensors_width_slices(input))) };
            else if (axis_dim == 2)
                return { concatenate_tensors_height(transform_slice_with_idx(tensor_to_tensors_height_slices(input))) };
            else if (axis_dim == 3)
                return { concatenate_tensors_dim4(transform_slice_with_idx(tensor_to_tensors_dim4_slices(input))) };
            else if (axis_dim == 4)
                return { concatenate_tensors_dim5(transform_slice_with_idx(tensor_to_tensors_dim5_slices(input))) };
            else
                raise_error("Invalid axis (" + std::to_string(axis_dim) + ") for Normalization layer");
            return {};
        }
        const std::vector<int> axes_;
        const float_vec mean_;
        const float_vec variance_;
        const bool invert_;
    };

}
}
