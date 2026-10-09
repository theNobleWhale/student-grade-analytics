#pragma once

#include <charconv>
#include <cmath>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

/// Union[int, float, str] equivalent: a score can arrive as an integer,
/// a floating-point number, or text read from a file.
using ScoreInput = std::variant<int, double, std::string>;

/// Converts any ScoreInput to a validated 0..100 integer score.
/// Doubles and decimal strings are rounded to the nearest integer.
/// Returns std::nullopt for blank, non-numeric, NaN or out-of-range input.
inline std::optional<int> toScore(const ScoreInput& input)
{
	auto validate = [](double value) -> std::optional<int>
		{
			if (!std::isfinite(value))
				return std::nullopt;

			long long rounded = std::llround(value);

			if (rounded < 0 || rounded > 100)
				return std::nullopt;

			return static_cast<int>(rounded);
		};

	return std::visit(
		[&](const auto& value) -> std::optional<int>
		{
			using T = std::decay_t<decltype(value)>;

			if constexpr (std::is_same_v<T, std::string>)
			{
				std::size_t begin = value.find_first_not_of(" \t\r\n");

				if (begin == std::string::npos)
					return std::nullopt;

				std::size_t end = value.find_last_not_of(" \t\r\n") + 1;

				double parsed = 0.0;
				const char* first = value.data() + begin;
				const char* last = value.data() + end;

				auto [ptr, ec] = std::from_chars(first, last, parsed);

				if (ec != std::errc{} || ptr != last)
					return std::nullopt;

				return validate(parsed);
			}
			else
			{
				return validate(static_cast<double>(value));
			}
		},
		input);
}
