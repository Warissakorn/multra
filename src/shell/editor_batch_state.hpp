#pragma once
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <thread>

class QLabel;
class QLineEdit;
class QTableWidget;
class QWidget;
namespace multra {
// M5.6 / M5.8b (BATCH §6-§7), D514: the batch and comparison worker's state and widgets, moved out of
// EditorWindow as the first step of its split (D505). The functions stay EditorWindow members
// (editor_batch.cpp, editor_compare.cpp); the results they install (`batch_`, `comparison_`) and
// their export markings stay on the window, which the result getters read.
struct BatchSession {
    QLineEdit* seeds{};
    QLabel* note{};
    QWidget* view{};
    QTableWidget *movementTable{}, *sectionTable{}, *queueTable{};
    // Every live worker by generation; each is joined by its own finishBatch, or at teardown. A
    // generation number discards whatever a cancelled or replaced batch posts back.
    struct Worker { std::thread thread; std::shared_ptr<std::atomic<bool>> cancel; };
    std::map<std::uint64_t, Worker> workers;
    std::shared_ptr<std::atomic<bool>> cancel;
    std::uint64_t generation{};
    std::size_t done{}, total{}; // total is non-zero while a batch runs
    bool comparing{};            // the running job is a comparison
    QWidget* compareView{};
    QLabel* compareNote{};
    QTableWidget *compareMovementTable{}, *compareSectionTable{}, *compareQueueTable{};
};
}
