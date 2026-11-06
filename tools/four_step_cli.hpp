#pragma once
// The four-step CLI runs (M10.2-M10.4): --trip-generation, --trip-distribution and --mode-choice, each with
// --project. Moved out of run_simulation.cpp (D478) to keep both files under the size limit.
#include <filesystem>
#include <string>
#include <vector>

namespace multra {
int runTripGenerationProject(const std::filesystem::path& file, const std::filesystem::path& csvFile, const std::filesystem::path& data,
                             const std::string& rateSet);
int runTripDistributionProject(const std::filesystem::path& file, const std::filesystem::path& csvFile, const std::filesystem::path& data,
                               const std::string& rateSet, const std::string& deterrence, const std::string& mode, const std::string& matrix);
int runModeChoiceProject(const std::filesystem::path& file, const std::filesystem::path& csvFile, const std::filesystem::path& data,
                         const std::string& rateSet, const std::string& deterrence, const std::string& mode, const std::string& matrix,
                         const std::string& modeChoiceSet, const std::vector<std::string>& periods);
}
